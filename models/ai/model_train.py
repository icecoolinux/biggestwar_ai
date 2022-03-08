
from src.model import *
from src.dataset_train import *

root = "."
dir_episodes = root+"../../dataset/episodes"


import os
import time

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.preprocessing import MinMaxScaler










if not open_files_net():
	print("Error to try open net files")
	exit()




# Set training params

platform = 'cpu'
if torch.cuda.is_available():
    platform = 'cuda'

# For both
DROPOUT = 0.0
GAMMA = 0.99
LEARNING_RATE_POLICY = 0.001

# For training
epochs = 1
BATCH_SIZE_ONPOLICY = 128
LEARNING_RATE_VALUE = 0.001
EPSILON_VALUE = 0.001
N_ROLL_QVALUE = 4



net = Net(DROPOUT)
net = net.to(platform)

step = create_step_struct()

optimizer = torch.optim.RMSprop(net.parameters(), lr=LEARNING_RATE_POLICY)
optimizer_value = torch.optim.Adam(net.parameters(), lr=LEARNING_RATE_VALUE, eps=EPSILON_VALUE)

# Loss functions only for pre-train
lossfunc_actions = nn.CrossEntropyLoss() # nn.NLLLoss()
lossfunc_units = nn.CrossEntropyLoss() # nn.NLLLoss()
lossfunc_local = nn.BCEWithLogitsLoss() # nn.MSELoss()
lossfunc_global = nn.BCEWithLogitsLoss() # nn.MSELoss()

lossfunc_value = nn.MSELoss()










# Load weights last training model.
net.load_state_dict(torch.load(root+'/saves/final/model.pth'))
net.eval()
net.train()











# Function that return input and output for the network

## Used in train with experience
            
def get_actions_output_exp(state, output):
    output = F.log_softmax(output[0], dim=0) # Apply log_softmax
    index_action = pos_actions_by_name[state['action_name']]
    return output[index_action]

def get_units_output_exp(state, output):
    if state['action_name'] == 'buildUnit':
        output = F.log_softmax(output[0][3:], dim=0) # Get units output and apply log_softmax
        for x in pos_units_by_name:
            if x in step['params'] and step['params'][x] > 0.5:
                index = pos_units_by_name[x]
                if index >= 3:
                    return output[index-3]
    elif state['action_name'] == 'buildBuilding':
        output = F.log_softmax(output[0][:3], dim=0) # Get building output and apply log_softmax
        for x in pos_units_by_name:
            if x in step['params'] and step['params'][x] > 0.5:
                index = pos_units_by_name[x]
                if index < 3:
                    return output[index]
    return None

def get_local_output_exp(state, output):
    index_first_action = index_localfirst(state['action_name'])
    index_second_action = index_localsecond(state['action_name'])
    
    first_local_t = None
    second_local_t = None
    
    if index_first_action >= 0 and index_second_action < 0:
        first_local_t = output[0, index_first_action, :, :]
        first_local_t = first_local_t.view(-1) # Flat to one dim
        first_local_t = F.log_softmax(first_local_t, dim=0) # Calculate log_softmax
        first_local_t = first_local_t.view(SIDE_LAYERS, SIDE_LAYERS) # Reshape to a matrix
        x1 = int(state['params']['x1Local'])
        y1 = int(state['params']['y1Local'])
        y1 = SIDE_LAYERS -y1 -1
        first_local_t = first_local_t[y1, x1] # Select the action pixel
    if index_second_action >= 0 and index_first_action < 0:
        second_local_t = output[0, index_second_action, :, :]
        second_local_t = second_local_t.view(-1) # Flat to one dim
        second_local_t = F.log_softmax(second_local_t, dim=0) # Calculate log_softmax
        second_local_t = second_local_t.view(SIDE_LAYERS, SIDE_LAYERS) # Reshape to a matrix
        x2 = int(state['params']['x2Local'])
        y2 = int(state['params']['y2Local'])
        y2 = SIDE_LAYERS -y2 -1
        second_local_t = second_local_t[y2, x2] # Select the action pixel
        
    return first_local_t, second_local_t

def get_global_output_exp(state, output):
    ratio = SIDE_MAP/SIDE_MINIMAP
    x1 = int(state['params']['x1NextGlobal']/ratio)
    y1 = int(state['params']['y1NextGlobal']/ratio)
    x2 = int(state['params']['x2NextGlobal']/ratio)
    y2 = int(state['params']['y2NextGlobal']/ratio)

    if x1 < 0:
        x1 = 0
    if x1 >= SIDE_MINIMAP:
        x1 = SIDE_MINIMAP-1
    if x2 < 0:
        x2 = 0
    if x2 >= SIDE_MINIMAP:
        x2 = SIDE_MINIMAP-1
    if y1 < 0:
        y1 = 0
    if y1 >= SIDE_MINIMAP:
        y1 = SIDE_MINIMAP-1
    if y2 < 0:
        y2 = 0
    if y2 >= SIDE_MINIMAP:
        y2 = SIDE_MINIMAP-1
    
    y1 = SIDE_MINIMAP -y1 -1
    y2 = SIDE_MINIMAP -y2 -1
    
    index_action = index_global(state['action_name'])
    
    first_global_t = output[0, index_action*2, :, :]
    first_global_t = first_global_t.view(-1) # Flat to one dim
    first_global_t = F.log_softmax(first_global_t, dim=0) # Calculate log_softmax
    first_global_t = first_global_t.view(SIDE_MINIMAP, SIDE_MINIMAP) # Reshape to a matrix
    first_global_t = first_global_t[x1, y1] # Select the action pixel
    
    second_global_t = output[0, index_action*2+1, :, :]
    second_global_t = second_global_t.view(-1) # Flat to one dim
    second_global_t = F.log_softmax(second_global_t, dim=0) # Calculate log_softmax
    second_global_t = second_global_t.view(SIDE_MINIMAP, SIDE_MINIMAP) # Reshape to a matrix
    second_global_t = second_global_t[x2, y2] # Select the action pixel
    
    return first_global_t, second_global_t













import time

net.set_value_return(True)

amount_episodes = episodes_from_exp()
set_epochs_exp(epochs)

for epoch in range(epochs):
    print("Epoch "+str(epoch))
    
    for index_e in range(amount_episodes):
        print("Episode "+str(index_e))

        # Init for metrics
        training_actions_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        training_units_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        training_local1_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        training_local2_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        training_global1_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        training_global2_loss = {'value': 0.0, 'min': float('+inf'), 'max': float('-inf')}
        amount_units_loss = 0
        amount_local1_loss = 0
        amount_local2_loss = 0
        amount_global1_loss = 0
        amount_global2_loss = 0
        
        # Reset hidden state
        net.reset_states()
        
        # Prefetch whole episode
        print("Start prefetch...")
        prefetch_next_episode_exp()
        print("End prefetch...")
        
        steps = get_steps_episode_exp()
        
        # Rewards and partial Q (until 4 step to the future)
        rewards, Q_partial = get_rewards_and_return_exp(GAMMA, N_ROLL_QVALUE)
        
        # Get perdicted values from net.
        predicted_values = []
        net.set_value_return(True)
        net.set_policy_return(False)
        for index in range(steps):
            get_step_exp(index, step) # get step
            # Get inputs
            dense_scalar_input = step['state_scalar'].to(platform)
            conv_layers_input = step['state_layers'].to(platform)
            conv_minimap_input = step['state_minimaps'].to(platform)
            # Forward only value
            value_out = net(dense_scalar_input, conv_layers_input, conv_minimap_input)
            predicted_values.append(value_out.item())
        
        # Calculate Q with predicted values as baseline
        Q_values = []
        for i in range(len(Q_partial)):
            pos_v_pred = i+N_ROLL_QVALUE
            q = Q_partial[i]
            if pos_v_pred < len(Q_partial):
                q += (GAMMA ** N_ROLL_QVALUE) * predicted_values[pos_v_pred]
            Q_values.append(q)
        
        #Q_values = np.ones(len(Q_partial))
        
        # Reset hidden state again
        net.reset_states()
        
        # Reset gradient
        net.zero_grad()
        
        # Forward and backpropagation count before to update weights
        batch_count = 0
        
        # Loop all state-action from episode
        net.set_value_return(True)
        net.set_policy_return(True)
        for index in range(steps):
            
            # Size of current batch (the last batch sure is smaller)
            current_batch_size = BATCH_SIZE_ONPOLICY
            if index >= (index//BATCH_SIZE_ONPOLICY)*BATCH_SIZE_ONPOLICY:
                current_batch_size = steps - (index//BATCH_SIZE_ONPOLICY)*BATCH_SIZE_ONPOLICY
            
            # Get step
            get_step_exp(index, step)
            
            # Get inputs
            dense_scalar_input = step['state_scalar'].to(platform)
            conv_layers_input = step['state_layers'].to(platform)
            conv_minimap_input = step['state_minimaps'].to(platform)

            # Forward
            actions_out, units_out, local_out, global_out, value_out = net(dense_scalar_input, conv_layers_input, conv_minimap_input)
            value_out = value_out[0,0]
            
            # Get target and prepare output (apply log_softmax and return only the taken action)
            action_out_exp = get_actions_output_exp(step, actions_out)
            unit_out_exp = get_units_output_exp(step, units_out)
            local1_out, local2_out = get_local_output_exp(step, local_out)
            global1_out, global2_out = get_global_output_exp(step, global_out)
            
            # Loss value and advantage of current action
            value_target = torch.FloatTensor([Q_values[index]]).to(platform)
            loss_value_v = lossfunc_value(value_out, value_target) / current_batch_size
            adv_value = value_target - value_out.detach() # Advantage = Q(s,a) - V(s)
            
            #print("Value target: "+str(value_target.item())+", value out: "+str(value_out.item())+", loss: "+str(loss_value_v.item()))
            
            
            # Calculate loss
            loss_unit = None
            loss_local1 = None
            loss_local2 = None
            loss_action = -(adv_value * action_out_exp) / current_batch_size
            if unit_out_exp is not None:
                loss_unit = -(adv_value * unit_out_exp) / current_batch_size
            if local1_out is not None:
                loss_local1 = -(adv_value * local1_out) / current_batch_size
            if local2_out is not None:
                loss_local2 = -(adv_value * local2_out) / current_batch_size
            loss_global1 = -(adv_value * global1_out) / current_batch_size
            loss_global2 = -(adv_value * global2_out) / current_batch_size
            
            # Backpropagate errors
            loss_action.backward(retain_graph=True)
            if unit_out_exp is not None:
                loss_unit.backward(retain_graph=True)
            if loss_local1 is not None:
                loss_local1.backward(retain_graph=True)
            if loss_local2 is not None:
                loss_local2.backward(retain_graph=True)
            loss_global1.backward(retain_graph=True)
            loss_global2.backward(retain_graph=True)
            
            loss_value_v.backward(retain_graph=True)

            # Is batch end or episode end? Update weights
            batch_count += 1
            if batch_count >= BATCH_SIZE_ONPOLICY or index == (steps-1):
                batch_count = 0
                optimizer.step() # Update weights
                net.zero_grad() # Reset gradient
            
            # Update hidden state
            net.swap_hidden_state()


            
            
            
            
            
            
            
            
            

net.reset_states()

net.set_value_return(True)
net.set_policy_return(True)
    
torch.save(net.state_dict(), root+'/saves/final/model.pth')




