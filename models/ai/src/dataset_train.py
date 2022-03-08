

from .config import *

import numpy as np
import pandas as pd
from PIL import Image

import torch
import torch.nn as nn
import torch.nn.functional as F


### Episode manager.

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
#         For each action (noaction, update, move, recollect, buildBuilding, buildUnit, attack, cancelAction):
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
    


def get_img_and_resize_(dir_episodes, num_episode, num_step, name, new_size):
	data_path = dir_episodes +"/"+ str(num_episode) +"/"+ str(num_step) +"/"+ name + ".png"
	img = Image.open(data_path)
	img = img.resize(new_size, Image.NEAREST) 
	img_array = np.asarray(img)
	img_array = np.array(img_array).astype('float32')/255.
	return img_array

def get_img_(dir_episodes, num_episode, num_step, name):
	data_path = dir_episodes +"/"+ str(num_episode) +"/"+ str(num_step) +"/"+ name + ".png"
	img = Image.open(data_path)
	img_array = np.asarray(img)
	img_array = np.array(img_array).astype('float32')/255.
	return img_array

def create_step_struct_():
	step = {}
	step['state_layers_np'] = np.empty([1, (CANT_LAYERS_PLAYERS*3 +CANT_LAYERS_MAPS)*2, SIDE_LAYERS, SIDE_LAYERS])
	step['state_minimaps_np'] = np.empty([1, CANT_MINIMAPS,SIDE_MINIMAP, SIDE_MINIMAP])
	return step

def get_step_(dir_episodes, num_episode, num_step, step):
    f = open(dir_episodes +"/"+ str(num_episode) +"/"+ str(num_step) +"/scalars.json")
    scalars =json.load(f)
    f.close()
        
    # State
    step['state_scalar'] = [scalars['timestamp'], scalars['local']['minerals'], scalars['local']['oils']]

    # I use threads
    index_layer = 0
    for a in ['current', 'aliades', 'enemies']:
        for b in ["base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"]:
            img = get_img_(dir_episodes, num_episode, num_step, 'state_'+a+'_first_'+b)
            step['state_layers_np'][0,index_layer,:,:] = img[:,:]
            index_layer += 1
    img = get_img_(dir_episodes, num_episode, num_step, 'state_objectmap_first_mineral')
    step['state_layers_np'][0,index_layer,:,:] = img[:,:]
    index_layer += 1
    img = get_img_(dir_episodes, num_episode, num_step, 'state_objectmap_first_baseszones')
    step['state_layers_np'][0,index_layer,:,:] = img[:,:]
    index_layer += 1
    
    for a in ['current', 'aliades', 'enemies']:
        for b in ["base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"]:
            img = get_img_(dir_episodes, num_episode, num_step, 'state_'+a+'_second_'+b)
            step['state_layers_np'][0,index_layer,:,:] = img[:,:]
            index_layer += 1
    img = get_img_(dir_episodes, num_episode, num_step, 'state_objectmap_second_mineral')
    step['state_layers_np'][0,index_layer,:,:] = img[:,:]
    index_layer += 1
    img = get_img_(dir_episodes, num_episode, num_step, 'state_objectmap_second_baseszones')
    step['state_layers_np'][0,index_layer,:,:] = img[:,:]

    index_layer = 0
    for a in ["minimapR", "minimapG", "minimapB", "update", "first", "second", "currentclosed", "futureclosed", "pixelstimeupdated"]:
        img = get_img_(dir_episodes, num_episode, num_step, 'state_global_'+a)
        step['state_minimaps_np'][0,index_layer,:,:] = img[:,:]
        index_layer += 1

    step['state_scalar'] = torch.FloatTensor([step['state_scalar']])
    step['state_layers'] = torch.FloatTensor(step['state_layers_np'])
    step['state_minimaps'] = torch.FloatTensor(step['state_minimaps_np'])

    # Action
    step['action_name'] = scalars['action']['name']  
    step['params'] = {}
    for n in scalars['action']:
        if n != 'name' and (n in pos_units_by_name or n in pos_local_by_name):
            step['params'][n] = scalars['action'][n]
    for n in scalars:
        if n in pos_global_by_name:
            step['params'][n] = scalars[n]
                
    if NORMALIZE_POSITIONS_ACTION:
        for n in pos_local_by_name:
            step['params'][n] /= SIDE_LAYERS
            step['params'][n] -= 0.5
        for n in pos_global_by_name:
            step['params'][n] /= SIDE_MAP
            step['params'][n] -= 0.5

    return step

def get_pos_of_local_and_global_(local_output, global_output):
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
    
def save_image_of_local_and_global_(local_output, global_output, name):
	for a in pos_actions_by_name:
		ind = index_localfirst(a)
		if ind >= 0:
			values = local_output[ind,:,:]
			values = values.detach().numpy()
			im = Image.fromarray(values * 255)
			im = im.convert('RGB')
			im.save(root+"/saves/images/local_"+name+"_"+a+"_first.png")
		ind = index_localsecond(a)
		if ind >= 0:
			values = local_output[ind,:,:]
			values = values.detach().numpy()
			im = Image.fromarray(values * 255)
			im = im.convert('RGB')
			im.save(root+"/saves/images/local_"+name+"_"+a+"_second.png")
		
		ind = index_global(a)
		if ind >= 0:
			values = global_output[2*ind,:,:]
			values = values.detach().numpy()
			im = Image.fromarray(values * 255)
			im = im.convert('RGB')
			im.save(root+"/saves/images/global_"+name+"_"+a+"_first.png")
			values = global_output[2*ind+1,:,:]
			values = values.detach().numpy()
			im = Image.fromarray(values * 255)
			im = im.convert('RGB')
			im.save(root+"/saves/images/global_"+name+"_"+a+"_second.png")

def get_action_from_output_(actions, units, local, global_sel):

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

fd_read = None
fd_write = None
def open_files_net_():
	global fd_read
	global fd_write
	try:
		fd_read = open("/tmp/fifo_bw_topython", "rb")
		fd_write = open("/tmp/fifo_bw_tocpp", "wb")
	except:
		print("Error to try open episodes experience fifo files")
		fd_read = None
		fd_write = None
		return False
	return True

def episodes_from_net():
	global fd_read
	global fd_write
	
	fd_write.write(b'episodes\n')
	fd_write.flush()

	buf = ""
	while True:
		byte = fd_read.read(1)
		c = byte.decode("utf-8")
		if c == '\n':
			byte = fd_read.read(1) # Read the end of the line '\0'
			break
		else:
			buf += c
			
	return int(str(buf))

def set_epochs_net(epochs):
	global fd_read
	global fd_write
	
	fd_write.write(b'epochs,'+bytes(str(epochs), encoding='utf8')+b'\n')
	fd_write.flush()

def get_step_from_net():
	global fd_read
	global fd_write
	
	buf = ""

	index = None
	isLast = None
	jsonScalar = None
	localSize = None
	globalSize = None

	while True:
		byte = fd_read.read(1)
		c = byte.decode("utf-8")
		if c == '\n':
			byte = fd_read.read(1) # Read the end of the line '\0'
			c = byte.decode("utf-8")
			if index is None:
				index = int(buf)
			elif isLast is None:
				isLast = (int(buf) == 1)
			elif jsonScalar is None:
				jsonScalar = json.loads(str(buf))
			elif localSize is None:
				localSize = int(buf)
			else:
				globalSize = int(buf)
				break
			buf = ""
		else:
			buf += c

	bytes_local = fd_read.read(localSize)
	bytes_global = fd_read.read(globalSize)

	local_np = np.frombuffer(bytes_local, dtype='float32')
	global_np = np.frombuffer(bytes_global, dtype='float32')

	return index, isLast, jsonScalar, local_np, global_np

episode_prefetched = []

def prefetch_next_episode_net():
	global fd_read
	global fd_write
	global episode_prefetched

	episode_prefetched = []
	isLast = False

	fd_write.write(b'start_steps\n')
	fd_write.flush()

	while not isLast:
		index, isLast, jsonScalar, local_np, global_np = get_step_from_net()
		episode_prefetched.append({'scalar':jsonScalar, 'local':local_np, 'global':global_np})

def get_rewards_and_return_net(gamma, N):
	steps = len(episode_prefetched)
	if N == -1:
		N = steps
	rewards = []
	for step in reversed(range(steps)):
		rew = 0
		if step == steps-1: # Last step (end of the episode)
			if episode_prefetched[step]['scalar']['endgame']['win'] == 1:
				rew = 1
			else:
				rew = -1
		rew += episode_prefetched[step]['scalar']['enemies_deads']
		rewards.insert(0, rew)
	returns = []
	for step in range(steps):
		ret = 0
		for i in reversed(range(step, step+N)):
			if i < steps:
				ret = rewards[i] + gamma*ret
		returns.append(ret)
	return rewards, returns

def get_step_net(num_step, step):
	scalars = episode_prefetched[num_step]['scalar']

	# State, local and global
	step['state_scalar'] = [scalars['timestamp'], scalars['local']['minerals'], scalars['local']['oils']]

	step['state_scalar'] = torch.FloatTensor([step['state_scalar']])


	#step['state_layers'] = torch.FloatTensor(episode_prefetched[num_step]['local']) # TODO reshape?
	#step['state_minimaps'] = torch.FloatTensor(episode_prefetched[num_step]['global']) # TODO reshape?

	local_np = episode_prefetched[num_step]['local'].reshape(( 1, ((3*CANT_LAYERS_PLAYERS+CANT_LAYERS_MAPS)*2), SIDE_LAYERS, SIDE_LAYERS))
	global_np = episode_prefetched[num_step]['global'].reshape(( 1, CANT_MINIMAPS, SIDE_MINIMAP, SIDE_MINIMAP))

	step['state_layers_np'][:,:,:,:] = local_np[:,:,:,:]
	step['state_minimaps_np'][:,:,:,:] = global_np[:,:,:,:]

	step['state_layers'] = torch.FloatTensor(step['state_layers_np'])
	step['state_minimaps'] = torch.FloatTensor(step['state_minimaps_np'])



	# Action
	step['action_name'] = scalars['action']['name']  
	step['params'] = {}
	for n in scalars['action']:
		if n != 'name' and (n in pos_units_by_name or n in pos_local_by_name):
			step['params'][n] = scalars['action'][n]
	for n in scalars:
		if n in pos_global_by_name:
			step['params'][n] = scalars[n]
				
	if NORMALIZE_POSITIONS_ACTION:
		for n in pos_local_by_name:
			step['params'][n] /= SIDE_LAYERS
			step['params'][n] -= 0.5
		for n in pos_global_by_name:
			step['params'][n] /= SIDE_MAP
			step['params'][n] -= 0.5

	return step
















# API to read the episodes

def get_step(dir_episodes, num_episode, num_step, step):
    return get_step_(dir_episodes, num_episode, num_step, step)


def open_files_net():
	return open_files_net_()

def create_step_struct():
	return create_step_struct_()

def set_epochs_exp(epochs):
	return set_epochs_net(epochs)
    
def episodes_from_exp():
	return episodes_from_net()

def prefetch_next_episode_exp():
	prefetch_next_episode_net()
    
def get_steps_episode_exp():
	return len(episode_prefetched)

def get_step_exp(num_step, step):
	return get_step_net(num_step, step)

# N is the count of rewards to future to calculate return, -1 is all
def get_rewards_and_return_exp(gamma, N):
	return get_rewards_and_return_net(gamma, N)



def get_pos_of_local_and_global(local_output, global_output):
	return get_pos_of_local_and_global_(local_output, global_output)
    
def save_image_of_local_and_global(local_output, global_output, name):
	return save_image_of_local_and_global_(local_output, global_output, name)

def get_action_from_output(actions, units, local, global_sel):
	return get_action_from_output_(actions, units, local, global_sel)















