
from .config import *

import numpy as np
import pandas as pd
from PIL import Image

import torch
import torch.nn as nn
import torch.nn.functional as F


### Episode manager.

import stat
import os
import json
from os import listdir
from os.path import isdir, isfile, join

# List of state-action.
# State: 
#        Scalar: timestamp, mineral, oil
#        Layers x2 (first and second) view:
#              Layers x3 (current, aliados, enemigos):
#                    Layers 128x128: Base, Barraca, Torreta, Recolector, SoldadoRaso, SoldadoEntrenado, 
#                                    Tanque, TanquePesado, Life, Creada, estaConstruyendoUnidad, Moviendose,
#                                    Recolectando, ConstruyendoBuild, Atacando, YendoAtacar.
#              Layer Map 128x128: AmountMineral, BasesZones
#        Minimap 256x256: MinimapR, MinimapG, MinimapB, FuturoCercano, AreaLocal, 
#                         First, Second, ActualCerrando, PixelsTimeUpdated
#
# Action: 
#         For each action (noaction, surrender, update, move, recollect, buildBuilding, buildUnit, attack, cancelAction):
#               Probability action (1)
#               Unit Type (8): Base, Barraca, Torreta, Recolector, SoldadoRaso, SoldadoEntrenado, Tanque, TanquePesado
#         For each action (noaction, surrender, update, move, recollect, buildBuilding, buildUnit, attack, cancelAction):
#               Local select first and second ( 2 x 128x128 )
#         For each action (noaction, surrender, update, move, recollect, buildBuilding, buildUnit, attack, cancelAction):
#               Global select first and second ( 2 x 256x256 )


# 'state_scalar', 'state_first_layers', 'state_second_layers', 'state_minimaps'
# 'action_name', 'params'(dict of pair param-value)

pos_actions_by_name = {'noAction':0, 'surrender':1, 'update':2, 'move':3, 'recollect':4, 'buildBuilding':5, 'buildUnit':6, 'attack':7, 'cancelAction':8}
name_by_pos_actions = {0:'noAction', 1:'surrender', 2:'update', 3:'move', 4:'recollect', 5:'buildBuilding', 6:'buildUnit', 7:'attack', 8:'cancelAction'}
pos_units_by_name = {"base":0, "barraca":1, "torreta":2, "recolector":3, "soldadoRaso":4, "soldadoEntrenado":5, "tanque":6, "tanquePesado":7}
name_by_pos_units = {0:"base", 1:"barraca", 2:"torreta", 3:"recolector", 4:"soldadoRaso", 5:"soldadoEntrenado", 6:"tanque", 7:"tanquePesado"}
pos_local_by_name = {"x1Local":0, "y1Local":1, "x2Local":2, "y2Local":3}
pos_global_by_name = {"x1NextGlobal":0, "y1NextGlobal":1, "x2NextGlobal":2, "y2NextGlobal":3}

def index_localfirst(action_name):
    if action_name == 'update':
        return 0
    elif action_name == 'move':
        return 1
    elif action_name == 'recollect':
        return 3
    elif action_name == 'buildUnit':
        return 5
    elif action_name == 'buildBuilding':
        return 6
    elif action_name == 'attack':
        return 8
    elif action_name == 'cancelAction':
        return 10
    else:
        return -1

def index_localsecond(action_name):
    if action_name == 'move':
        return 2
    elif action_name == 'recollect':
        return 4
    elif action_name == 'buildBuilding':
        return 7
    elif action_name == 'attack':
        return 9
    else:
        return -1
        
def index_global(action_name):
    if action_name == 'noAction':
        return 0
    elif action_name == 'update':
        return 1
    elif action_name == 'move':
        return 2
    elif action_name == 'recollect':
        return 3
    elif action_name == 'buildUnit':
        return 4
    elif action_name == 'buildBuilding':
        return 5
    elif action_name == 'attack':
        return 6
    elif action_name == 'cancelAction':
        return 7
    else:
        return -1

def create_step_struct():
    step = {}
    step['state_layers_np'] = np.empty([1, (CANT_LAYERS_PLAYERS*3 +CANT_LAYERS_MAPS)*2, SIDE_LAYERS, SIDE_LAYERS])
    step['state_minimaps_np'] = np.empty([1, CANT_MINIMAPS,SIDE_MINIMAP, SIDE_MINIMAP])
    state = {}
    state['cell_np'] = np.empty([2, 1, 64])
    state['hidden_np'] = np.empty([2, 1, 64])
    return step, state

def get_pos_of_local_and_global(local_output, global_output):
    ret = {}
    for ind, a in enumerate(pos_actions_by_name):
        ret[a] = {}
        
        ret[a]['local'] = {}
        index_max = np.argmax(local_output[:,:,2*ind])
        x_first = index_max % SIDE_LAYERS
        y_first = index_max // SIDE_LAYERS
        y_first = SIDE_LAYERS -y_first -1
        index_max = np.argmax(local_output[:,:,2*ind+1])
        x_second = index_max % SIDE_LAYERS
        y_second = index_max // SIDE_LAYERS
        y_second = SIDE_LAYERS -y_second -1
        ret[a]['local']['first'] = (x_first, y_first)
        ret[a]['local']['second'] = (x_second, y_second)
        
        ret[a]['global'] = {}
        index_max = np.argmax(global_output[:,:,2*ind])
        x_first = index_max % SIDE_MINIMAP
        y_first = index_max // SIDE_MINIMAP
        y_first = SIDE_MINIMAP -y_first -1
        index_max = np.argmax(global_output[:,:,2*ind+1])
        x_second = index_max % SIDE_MINIMAP
        y_second = index_max // SIDE_MINIMAP
        y_second = SIDE_MINIMAP -y_second -1
        ret[a]['global']['first'] = (x_first, y_first)
        ret[a]['global']['second'] = (x_second, y_second)
        
    return ret
    
def save_image_of_layers_and_minimaps(path, layers, minimaps, name):
	for a in range((3*CANT_LAYERS_PLAYERS+CANT_LAYERS_MAPS)*2):
		values = layers[0,a,:,:]
		im = Image.fromarray(values * 255)
		im = im.convert('RGB')
		im.save(path+"layers_"+name+"_"+str(a)+".png")
	for a in range(CANT_MINIMAPS):
		values = minimaps[0,a,:,:]
		im = Image.fromarray(values * 255)
		im = im.convert('RGB')
		im.save(path+"minimaps_"+name+"_"+str(a)+".png")
            
def save_image_of_local_and_global(path, local_output, global_output, name):
    for a in pos_actions_by_name:
        ind = index_localfirst(a)
        if ind >= 0:
            values = local_output[ind,:,:]
            im = Image.fromarray(values * 255)
            im = im.convert('RGB')
            im.save(path+"local_"+name+"_"+a+"_first.png")
        ind = index_localsecond(a)
        if ind >= 0:
            values = local_output[ind,:,:]
            im = Image.fromarray(values * 255)
            im = im.convert('RGB')
            im.save(path+"local_"+name+"_"+a+"_second.png")
        
        ind = index_global(a)
        if ind >= 0:
            values = global_output[2*ind,:,:]
            im = Image.fromarray(values * 255)
            im = im.convert('RGB')
            im.save(path+"global_"+name+"_"+a+"_first.png")
            values = global_output[2*ind+1,:,:]
            im = Image.fromarray(values * 255)
            im = im.convert('RGB')
            im.save(path+"global_"+name+"_"+a+"_second.png")

def get_action_from_output(actions, units, local, global_sel):

    index_action = np.argmax(actions)
    
    index_unit = -1
    if pos_actions_by_name['buildBuilding'] == index_action:
        index_unit = np.argmax(units[:3])
    elif pos_actions_by_name['buildUnit'] == index_action:
        index_unit = np.argmax(units[3:])+3
        
    action = ['noaction', 'surrender', 'update', 'move', 'recollect', 'buildbuilding', 'buildunit', 'attack', 'cancelaction'][index_action]
    unit_type = ['base', 'barraca', 'torreta', 'recolector', 'soldadoraso', 'soldadoentrenado', 'tanque', 'tanquepesado'][index_unit]
    
    ind = index_localfirst(name_by_pos_actions[index_action])
    x1 = -1
    y1 = -1
    if ind >= 0:
        index_max = np.argmax(local[ind,:,:])
        x1 = index_max % SIDE_LAYERS
        y1 = index_max // SIDE_LAYERS
    x2 = -1
    y2 = -1
    ind = index_localsecond(name_by_pos_actions[index_action])
    if ind >= 0:
        index_max = np.argmax(local[ind,:,:])
        x2 = index_max % SIDE_LAYERS
        y2 = index_max // SIDE_LAYERS
    
    ind = index_global(name_by_pos_actions[index_action])
    x1_g = -1
    y1_g = -1
    x2_g = -1
    y2_g = -1
    if ind >= 0:
        index_max = np.argmax(global_sel[2*ind,:,:])
        x1_g = index_max % SIDE_MINIMAP
        y1_g = index_max // SIDE_MINIMAP
        index_max = np.argmax(global_sel[2*ind+1,:,:])
        x2_g = index_max % SIDE_MINIMAP
        y2_g = index_max // SIDE_MINIMAP

    y1 = SIDE_LAYERS -y1 -1
    y2 = SIDE_LAYERS -y2 -1
    y1_g = SIDE_MINIMAP -y1_g -1
    y2_g = SIDE_MINIMAP -y2_g -1
    
    for x in pos_actions_by_name:
        print(x+": "+str(actions[pos_actions_by_name[x]]))

    unit_type = "None"
    if index_unit >= 0:
        unit_type = name_by_pos_units[index_unit]
        
    print(name_by_pos_actions[index_action]+" "+unit_type+", localfirst("+str(x1)+","+str(y1)+"), localsecond("+str(x2)+","+str(y2)+"), globalfirst("+str(x1_g)+","+str(y1_g)+"), globalsecond("+str(x2_g)+","+str(y2_g)+")")



# Read and send data from/to c++ 

import os
import struct
import array
import json

def get_step_from_net(fd):
    buf = ""
    
    jsonScalar = None
    localSize = None
    globalSize = None
    cellStateSize = None
    hiddenStateSize = None
    
    while True:
        byte = fd.read(1)
        c = byte.decode("utf-8")
        if c == '\n':
            byte = fd.read(1) # Read the end of the line '\0'
            c = byte.decode("utf-8")
            if jsonScalar is None:
                jsonScalar = json.loads(str(buf))
            elif localSize is None:
                localSize = int(buf)
            elif globalSize is None:
                globalSize = int(buf)
            elif cellStateSize is None:
                cellStateSize = int(buf)
            else:
                hiddenStateSize = int(buf)
                break
            buf = ""
        else:
            buf += c

    bytes_local = fd.read(localSize)
    bytes_global = fd.read(globalSize)
    bytes_cell_state = fd.read(cellStateSize)
    bytes_hidden_state = fd.read(hiddenStateSize)
    
    local_np = np.frombuffer(bytes_local, dtype='float32')
    global_np = np.frombuffer(bytes_global, dtype='float32')
    cell_state_np = np.frombuffer(bytes_cell_state, dtype='float32')
    hidden_state_np = np.frombuffer(bytes_hidden_state, dtype='float32')
    
    return jsonScalar, local_np, global_np, cell_state_np, hidden_state_np

def get_step(fd, step, state):
	
	ret = get_step_from_net(fd)
	
	if len(ret) == 1 and not ret:
		return False
	
	scalars, local_np, global_np, cell_state_np, hidden_state_np = ret

	# State, local and global
	step['state_scalar'] = [scalars['timestamp'], scalars['local']['minerals'], scalars['local']['oils']]

	step['state_scalar'] = torch.FloatTensor([step['state_scalar']])
	
	
	#step['state_layers'] = torch.FloatTensor(episode_prefetched[num_step]['local']) # TODO reshape?
	#step['state_minimaps'] = torch.FloatTensor(episode_prefetched[num_step]['global']) # TODO reshape?
	
	local_np = local_np.reshape(( 1, ((3*CANT_LAYERS_PLAYERS+CANT_LAYERS_MAPS)*2), SIDE_LAYERS, SIDE_LAYERS))
	global_np = global_np.reshape(( 1, CANT_MINIMAPS, SIDE_MINIMAP, SIDE_MINIMAP))
	cell_state_np = cell_state_np.reshape(( 2, 1, 64))
	hidden_state_np = hidden_state_np.reshape(( 2, 1, 64))
	
	step['state_layers_np'][:,:,:,:] = local_np[:,:,:,:]
	step['state_minimaps_np'][:,:,:,:] = global_np[:,:,:,:]
	state['cell_np'][:,:,:] = cell_state_np[:,:,:]
	state['hidden_np'][:,:,:] = hidden_state_np[:,:,:]
	
	step['state_layers'] = torch.FloatTensor(step['state_layers_np'])
	step['state_minimaps'] = torch.FloatTensor(step['state_minimaps_np'])
	state['cell'] = torch.FloatTensor(state['cell_np'])
	state['hidden'] = torch.FloatTensor(state['hidden_np'])
	
	return True


def send_action(fd, actions_out, units_out, local_out, global_out, cell_state, hidden_state):
	fd.write(actions_out.tobytes())
	fd.write(units_out.tobytes())
	fd.write(local_out.tobytes())
	fd.write(global_out.tobytes())
	fd.write(cell_state.tobytes())
	fd.write(hidden_state.tobytes())
	fd.flush()

	
	
# Handle the client's files
clients_files = []

def check_new_clients():
	try:
		while True:
			filename_read = "/tmp/fifo_bw_agent_topython_"+str(len(clients_files))
			filename_write = "/tmp/fifo_bw_agent_tocpp_"+str(len(clients_files))
			if (os.path.isfile(filename_read) or stat.S_ISFIFO(os.stat(filename_read).st_mode)) and (os.path.isfile(filename_write) or stat.S_ISFIFO(os.stat(filename_write).st_mode)):
				fd_read = open(filename_read, "rb")
				fd_write = open(filename_write, "wb")
				clients_files.append({'read':fd_read, 'write':fd_write})
			else:
				break
	except:
		print("Error check new clients")
		return



import pickle

def save_obj(obj, name ):
	with open(name + '.pkl', 'wb') as f:
		pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)

def load_obj(name ):
	with open(name + '.pkl', 'rb') as f:
		return pickle.load(f)
    
    


