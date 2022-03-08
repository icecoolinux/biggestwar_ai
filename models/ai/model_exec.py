
from src.model import *
from src.dataset_exec import *

root = "."
dir_episodes = root+"../../dataset/episodes"


import os
import time

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.preprocessing import MinMaxScaler




# Load model
platform = 'cpu'
if torch.cuda.is_available():
	platform = 'cuda'

net = Net(0.0)
net = net.to(platform)

step, state = create_step_struct()

net.load_state_dict(torch.load(root+'/saves/final/model.pth'))
net.eval()



net.set_value_return(True)
net.set_policy_return(True)

count_steps = 0

while True:
	check_new_clients()
	
	for client in clients_files:
	
		# Get step
		if get_step(client['read'], step, state):
			
			# Transform inputs
			dense_scalar_input = step['state_scalar'].to(platform)
			conv_layers_input = step['state_layers'].to(platform)
			conv_minimap_input = step['state_minimaps'].to(platform)
			cell_state = state['cell'].to(platform)
			hidden_state = state['hidden'].to(platform)
			
			#save_image_of_layers_and_minimaps("./saves/images/", conv_layers_input.clone().to('cpu').detach().numpy(), conv_minimap_input.clone().to('cpu').detach().numpy(), str(count_steps))
			
			# Set hidden state
			net.set_hidden_state(cell_state, hidden_state)
			
			# Foreward
			actions_out, units_out, local_out, global_out, value_out = net(dense_scalar_input, conv_layers_input, conv_minimap_input)
			
			#print(torch.argmax(global_out[:,2,:,:]))
			#print(torch.max(global_out[:,2,:,:]))
			
			# Get hidden state
			net.swap_hidden_state()
			cell_state_np, hidden_state_np = net.get_hidden_state()
			cell_state = cell_state_np.to('cpu').numpy() 
			hidden_state = hidden_state_np.to('cpu').numpy() 
			
			# Apply softmax and sigmoid
			actions_out = F.softmax(actions_out)
			local_out = F.sigmoid(local_out)
			global_out = F.sigmoid(global_out)
			
			# Get output to cpu memory
			actions_out = actions_out.to('cpu')[0].detach().numpy()
			units_out = units_out.to('cpu')[0].detach().numpy()
			local_out = local_out.to('cpu')[0].detach().numpy()
			global_out = global_out.to('cpu')[0].detach().numpy()
			
			#save_image_of_local_and_global("./saves/images/", local_out, global_out, str(count_steps))
			#print("Final intent (value "+str(value_out.item())+"):")
			#action, unit, local, global_ = get_action_from_output(actions_out, units_out, local_out, global_out)

			send_action(client['write'], actions_out, units_out, local_out, global_out, cell_state, hidden_state)
			
			count_steps += 1
			
	time.sleep(0.010)

