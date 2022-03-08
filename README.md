


RL

Install
sudo apt install libopencv-dev
sudo apt install libprotobuf-dev

make clean

make agentProg
make models/dataset/makeDataset
make agentIAtorch

or 

make


# Generate dataset from episodes
From the episodes saves from server this command export them to a dataset easy to use for AI algorithms.
The exported data is saved on dataset/episodes/

For export the episodes
cd dataset/
./makeDataset

Other options:

makeDataset --net --dir <dir to save> --no-save --replays <dir> --use-noaction --use-only-icecool

--dir <dir to save>: Default ./episodes
--replays <dir>: Default ../server/data/replays
--net: Default false
--use-noaction: Default false
--use-only-icecool: Default false
--no-save: Not save to disk




Gestiona un agente (un player) contra un mundo.
Levanta un modelo pre-entrenado y ejecuta.
Tambien puede activar el entrenamiento mientras está ejecutando.


Environment

Rewards:1 when the team win, -1 when lose. Too 1 when a enemy died.


Model

GAMMA = 0.99
LEARNING_RATE = 0.00001
epochs = 5
BATCH_SIZE_ONPOLICY = 128
N_ROLL_QVALUE = 4

Actor Critic
Q(s,a) = V(s) + A(s,a)

For each epoch, for each episode
Calculate Q(s,a) for each step:
Q(s,a) = Sum(0,n)(gamma*ri) + gamma^n * V(sn); N=4

For each step:

Get input, the state of the game
dense_scalar_input = step['state_scalar'].to(platform)
conv_layers_input = step['state_layers'].to(platform)
conv_minimap_input = step['state_minimaps'].to(platform)

Forward to the net and get action output and predicted return value
actions_out, units_out, local_out, global_out, value_out = net(dense_scalar_input, conv_layers_input, conv_minimap_input)
value_out = value_out[0,0]

Get target and prepare output (apply log_softmax and return only the taken action)
action_out_exp = get_actions_output_exp(step, actions_out)
unit_out_exp = get_units_output_exp(step, units_out)
local1_out, local2_out = get_local_output_exp(step, local_out)
global1_out, global2_out = get_global_output_exp(step, global_out)

Loss value and advantage of current action
value_target = torch.FloatTensor([Q_values[index]]).to(platform)
loss_value_v = lossfunc_value(value_out, value_target) / BATCH_SIZE_ONPOLICY
adv_value = value_target - value_out.detach() # Advantage = Q(s,a) - V(s)


Calculate loss
loss_unit = None
loss_local1 = None
loss_local2 = None
loss_action = -(adv_value * action_out_exp) / BATCH_SIZE_ONPOLICY
if unit_out_exp is not None:
	loss_unit = -(adv_value * unit_out_exp) / BATCH_SIZE_ONPOLICY
if local1_out is not None:
	loss_local1 = -(adv_value * local1_out) / BATCH_SIZE_ONPOLICY
if local2_out is not None:
	loss_local2 = -(adv_value * local2_out) / BATCH_SIZE_ONPOLICY
loss_global1 = -(adv_value * global1_out) / BATCH_SIZE_ONPOLICY
loss_global2 = -(adv_value * global2_out) / BATCH_SIZE_ONPOLICY

Backpropagate errors
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

            








Software
agentIAtorch: An agent that acts with a model neural network but the model execute in a isolation python process.

agentProg: A dummy agent that has rules to execute and act.

RL/dataset/makeDataset: An usefull software that generates dataset states for model training from episodes keeped on server directory.
Genera episodios para entrenar el agente offline a partir de los mensajes almacenados por el servidor.
O sea, transforma mensajes enviados a un dataset facil de leer para entrenar al agente.

Source code
RL/models/ai: The agent IA, using torch.
RL/models/ai/model.ipynb: Notebook with the research of the last model.
RL/models/prog: Implements agentProg, the dummy agent.

RL/models/episodes.zip: There are some episodes data for training.

Scripts
RL/scripts/generateEpisodesToPreTraining.py: It’s for generate a lot of episodes. Lunch serverMain, some serverWorld and a lot of agents, wait for agents and world to end. Repeat many times.

RL/scripts/generateEpisodesToTrainRL.py: It’s for train the model on policy way. Lunch serverMain, some serverWorld, a lot of agents IA torch that act from the current model and execute model_exec.py that recive the state and return the action. After train the model with the updated data with model_train.py. Repeat many times.

RL/scripts/generateOnlyAgents.py: Just lunch a lot of agents and wait for their ended.

RL/scripts/run.sh: Just execute a command in a remote server and prevent that the ended when I exit the ssh.

RL/models/ai/model_exec.py: Execute the model saved in ./saves/final/model.pth. Receive the states from /tmp/fifo_bw_agent_topython_%d and send action to /tmp/fifo_bw_agent_tocpp_%d.

RL/models/ai/model_train.py: Train the model in an online way. Read state from /tmp/fifo_bw_topython and send commands to /tmp/fifo_bw_tocpp.


















