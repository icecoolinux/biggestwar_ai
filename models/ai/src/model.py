
import torch
import torch.nn as nn
import torch.nn.functional as F

import numpy as np
import pandas as pd

from .config import *


start_neurons = 16

padding_mode = 'replicate'
#padding_mode = 'zeros'


# WARNING: la deconv2d tiene stride 2 y no tiene activation, me vino asi del otro
# WARNING: Conv2d local y global tiene kernel de 1x1
# WARNING: mejorar esta red dejando una red llamada base hasta el final de LSTM y un cabezal para cada salida,
#      el forwarding primero lo hago en la base y luego incovoco cada head
# Se tiene que hacer lo anterior de poner primero y una sola vez la base porque el estado de LSTM
# se debe ajustar una vez.

class Net(nn.Module):

    def __init__(self, dropout_amount):
        super(Net, self).__init__()
        # Layers input:   (SIDE_LAYERS, SIDE_LAYERS, (CANT_LAYERS_PLAYERS*3 +CANT_LAYERS_MAPS)*2)
        # Minimaps input: (SIDE_MINIMAP, SIDE_MINIMAP, CANT_MINIMAPS)
        # Scalars input: (3)
        #
        # Actions output: (CANT_ACTIONS)
        # Units output:   (CANT_UNITS)
        # Local output:   (11, SIDE_LAYERS, SIDE_LAYERS)
        # Global output:  ((CANT_ACTIONS-1)*2, SIDE_MINIMAP, SIDE_MINIMAP)

        self.dropout_amount = dropout_amount
        
        self.policy_return = True
        self.value_return = True
        
        # Make LSTM hidden state
        # The axes semantics are (num_layers, minibatch_size, hidden_dim)
        #self.hidden_state = (torch.zeros(2, 1, 64), torch.zeros(2, 1, 64))
        self.register_buffer('cell_state', torch.zeros(2, 1, 64))
        self.register_buffer('hidden_state', torch.zeros(2, 1, 64))
            
        self.make_first_part()
        self.make_middle_part()
        self.make_last_part()
        
        self.make_value_part()
    
    def set_policy_return(self, enable):
        self.policy_return = enable
    def set_value_return(self, enable):
        self.value_return = enable
        
    def make_first_part(self):
        # Dense scalar, scalar input
        self.dense_x = nn.Sequential(
            nn.Linear(3, 16),
            nn.Sigmoid()
        )
        
        # First Conv2D, minimaps input
        self.conv1 = nn.Sequential(
            nn.Conv2d(CANT_MINIMAPS, start_neurons*1, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*1, start_neurons*1, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        self.pool1 = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Dropout(self.dropout_amount)
        )
        
        # Conv2D, layers input
        self.conv1_layers = nn.Sequential(
            nn.Conv2d((CANT_LAYERS_PLAYERS*3 +CANT_LAYERS_MAPS)*2, start_neurons*1, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        
        # Concatenate minimaps and layers Conv2D
        # Concatenate: pool1 and conv1_layers, return pool1
        
        # Second Conv2D
        self.conv2 = nn.Sequential(
            nn.Conv2d(start_neurons*2, start_neurons*2, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*2, start_neurons*2, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        self.pool2 = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Dropout(self.dropout_amount)
        )
        
        # Third Conv2D
        self.conv3 = nn.Sequential(
            nn.Conv2d(start_neurons*2, start_neurons*4, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*4, start_neurons*4, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        self.pool3 = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Dropout(self.dropout_amount)
        )
        
        # Fourth Conv2D
        self.conv4 = nn.Sequential(
            nn.Conv2d(start_neurons*4, start_neurons*8, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*8, start_neurons*8, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        self.pool4 = nn.Sequential(
            nn.MaxPool2d(2),
            nn.Dropout(self.dropout_amount)
        )
        
    def make_middle_part(self):
        self.conv_middle = nn.Sequential(
            nn.Conv2d(start_neurons*8, start_neurons*16, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        
        # Flatten
        # To (minibatch, -1), return conv_middle_flatten
        
        # Concatenate scalar dense_x and flatten
        # Concatenate: dense_x and conv_middle_flatten, return x_concat
        
        # LSTM
        self.lstm_layer = nn.LSTM(input_size=start_neurons*16 * 16*16 + 16, 
                                  hidden_size=64, 
                                  num_layers=2, 
                                  dropout=self.dropout_amount)
        
        # Dense
        self.out_dense_preaction = nn.Sequential(
            nn.Linear(64, 64),
            nn.Sigmoid(),
            nn.Dropout(self.dropout_amount)
        )
        
        # Dense, output action
        self.output_actions = nn.Sequential(
            nn.Linear(64, CANT_ACTIONS)
        )

        # Dense, output units
        self.output_units = nn.Sequential(
            nn.Linear(64, CANT_UNITS)
        )
        
        # Dense
        self.dense_middle_output = nn.Sequential(
            nn.Linear(64, start_neurons*16 * 16*16),
            nn.Sigmoid()
        )

        # Reshape
        # To: (minibatch, start_neurons*16, 16, 16), return convm
        
    def make_last_part(self):
        # First DeConv2D
        self.deconv4 = nn.ConvTranspose2d(start_neurons*16, 
                                          start_neurons*8, 
                                          kernel_size=3, 
                                          padding=1, 
                                          output_padding=1, 
                                          stride=2)
        # Concatenate: deconv4 and conv4, return uconv4
        self.uconv4 = nn.Sequential(
            nn.Dropout(self.dropout_amount),
            nn.Conv2d(start_neurons*16, start_neurons*8, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*8, start_neurons*8, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        
        # Second DeConv2D
        self.deconv3 = nn.ConvTranspose2d(start_neurons*8, 
                                          start_neurons*4, 
                                          kernel_size=3, 
                                          padding=1, 
                                          output_padding=1, 
                                          stride=2)
        # Concatenate: deconv3 and conv3, return uconv3
        self.uconv3 = nn.Sequential(
            nn.Dropout(self.dropout_amount),
            nn.Conv2d(start_neurons*8, start_neurons*4, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*4, start_neurons*4, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        
        # Third DeConv2D
        self.deconv2 = nn.ConvTranspose2d(start_neurons*4, 
                                          start_neurons*2, 
                                          kernel_size=3, 
                                          padding=1, 
                                          output_padding=1, 
                                          stride=2)
        # Concatenate: deconv2 and conv2, return uconv2
        self.uconv2 = nn.Sequential(
            nn.Dropout(self.dropout_amount),
            nn.Conv2d(start_neurons*4, start_neurons*2, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*2, start_neurons*2, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU()
        )
        
        # Fourth DeConv2D
        self.deconv1 = nn.ConvTranspose2d(start_neurons*2, 
                                          start_neurons*1, 
                                          kernel_size=3, 
                                          padding=1, 
                                          output_padding=1, 
                                          stride=2)
        # Concatenate: deconv1 and conv1, return uconv1
        self.uconv1 = nn.Sequential(
            nn.Dropout(self.dropout_amount),
            nn.Conv2d(start_neurons*2, start_neurons*1, kernel_size=3, padding=1, padding_mode=padding_mode),
            nn.ReLU(),
            nn.Conv2d(start_neurons*1, start_neurons*1, kernel_size=3, padding=1, padding_mode=padding_mode)
        )

        # Conv2D, output local
        self.output_local = nn.Sequential(
            nn.Conv2d(start_neurons*2, 11, kernel_size=1, padding=0)
        )
        
        # Conv2D, output global
        self.output_global = nn.Sequential(
            nn.Conv2d(start_neurons*1, (CANT_ACTIONS-1)*2, kernel_size=1, padding=0)
        )
    
    def make_value_part(self):
        self.value = nn.Sequential(
            nn.Linear(start_neurons*16 * 16*16 + 16, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 1)
        )
        
    def reset_states(self):
        nn.init.zeros_(self.cell_state)
        nn.init.zeros_(self.hidden_state)
    
    def swap_hidden_state(self):
        self.cell_state = self.cell_state_out.clone().detach()
        self.hidden_state = self.hidden_state_out.clone().detach()
        #self.hidden_state[0][:] = self.hidden_state_out[0][:]
        #self.hidden_state[1][:] = self.hidden_state_out[1][:]
    
    def get_hidden_state(self):
        return (self.cell_state.clone().detach(), self.hidden_state.clone().detach())
    
    def set_hidden_state(self, cell_state, hidden_state):
        self.cell_state[:] = cell_state[:]
        self.hidden_state[:] = hidden_state[:]
        
    def forward(self, scalar_input, layers_input, global_input):
        output_actions_t = None
        output_units_t = None
        output_local_t = None
        output_global_t = None
        value_t = None
        
        # First part
        dense_x_t = self.dense_x(scalar_input)
        conv1_t = self.conv1(global_input)
        pool1_t = self.pool1(conv1_t)
        conv1_layers_t = self.conv1_layers(layers_input)
        pool1_t = torch.cat( (pool1_t, conv1_layers_t), dim=1)
        conv2_t = self.conv2(pool1_t)
        pool2_t = self.pool2(conv2_t)
        conv3_t = self.conv3(pool2_t)
        pool3_t = self.pool3(conv3_t)
        conv4_t = self.conv4(pool3_t)
        pool4_t = self.pool4(conv4_t)
        
        # Middle part
        conv_middle_t = self.conv_middle(pool4_t)
        conv_middle_flatten_t = conv_middle_t.view(1, -1) # First arg is minibatch
        x_concat_t = torch.cat( (dense_x_t, conv_middle_flatten_t), dim=1)
        x_concat_t = x_concat_t.view(1, 1, -1) # Add sequent dimension, second arg is minibatch
        if self.policy_return:
            lstm_layer_t, hidden_state_out = self.lstm_layer(x_concat_t, (self.cell_state, self.hidden_state) )
            self.cell_state_out = hidden_state_out[0]
            self.hidden_state_out = hidden_state_out[1]
            lstm_layer_t = lstm_layer_t.view(1, -1) # Delete sequent dimension, first arg is minibatch
            out_dense_preaction_t = self.out_dense_preaction(lstm_layer_t)
            dense_middle_output_t = self.dense_middle_output(out_dense_preaction_t)
            convm_t = dense_middle_output_t.view(1, start_neurons*16, 16, 16) # First arg is minibatch

            # Last part
            deconv4_t = self.deconv4(convm_t)
            uconv4_t = torch.cat( (deconv4_t, conv4_t), dim=1)
            uconv4_t = self.uconv4(uconv4_t)
            deconv3_t = self.deconv3(uconv4_t)
            uconv3_t = torch.cat( (deconv3_t, conv3_t), dim=1)
            uconv3_t = self.uconv3(uconv3_t)
            deconv2_t = self.deconv2(uconv3_t)
            uconv2_t = torch.cat( (deconv2_t, conv2_t), dim=1)
            uconv2_t = self.uconv2(uconv2_t)
            deconv1_t = self.deconv1(uconv2_t)
            uconv1_t = torch.cat( (deconv1_t, conv1_t), dim=1)
            uconv1_t = self.uconv1(uconv1_t)

            # Outputs
            output_actions_t = self.output_actions(out_dense_preaction_t)
            output_units_t = self.output_units(out_dense_preaction_t)
            output_local_t = self.output_local(uconv2_t)
            output_global_t = self.output_global(uconv1_t)
            
        # Value
        if self.value_return:
            value_t = self.value(x_concat_t)
        
        # Only policy
        if self.policy_return and not self.value_return:
            return output_actions_t, output_units_t, output_local_t, output_global_t
        # Only value
        elif not self.policy_return and self.value_return:
            return value_t
        # both
        else:
            return output_actions_t, output_units_t, output_local_t, output_global_t, value_t

    def num_flat_features(self, x):
        size = x.size()[1:]  # all dimensions except the batch dimension
        num_features = 1
        for s in size:
            num_features *= s
        return num_features
