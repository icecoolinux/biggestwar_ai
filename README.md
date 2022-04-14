

# BiggestWar - Reinforcement Learning AI

[![Watch the video](https://img.youtube.com/vi/q9bt5E9jACU/hqdefault.jpg)](https://www.youtube.com/watch?v=q9bt5E9jACU)

## How to start

### Install

```bash
sudo apt install libopencv-dev
sudo apt install libprotobuf-dev
```

### Compile
```bash
make clean

make agentProg
make models/dataset/makeDataset
make agentIAtorch

or 

make
```



## Reinforcement Learning
The agent is a player in the world of BiggestWar that loads a model pre-training and does actions.  
We can play with differents networks to reach the best performance :)  

### Environment
The world has 8000x8000 meters, there are a lot of minerals and three teams, each team can have one hundred players and each player can have two hundred buildings and two hundred units.  
Win the team that is the last in died.  

The Reward: 
* 1 when the team win at the end of the episode
* 1 when a enemy player died (I don't know if this reward can be good)
* -1 when lose (when all players of the team died)


### My Model

#### Parameters
GAMMA = 0.99  
LEARNING_RATE = 0.00001  
epochs = 5  
BATCH_SIZE_ONPOLICY = 128  
N_ROLL_QVALUE = 4  

#### Actor Critic
Q(s,a) = V(s) + A(s,a)

For each epoch, for each episode  
Calculate Q(s,a) for each step:  
Q(s,a) = Sum(0,n)(gamma*ri) + gamma^n * V(sn); N=4  

For each step:  
* Get input, the state of the game
* Forward to the net and get action output and predicted return value
* Get target and prepare output (apply log_softmax and return only the taken action)
* Loss value and advantage of current action
* Calculate loss
* Backpropagate errors
* Update weights and reset gradient (At the end of the batch)

## Binaries
**agentIAtorch**: An agent that acts with a model neural network but the model execute in a isolation python process.  

**agentProg**: A dummy agent that has rules to execute and act.  

**RL/dataset/makeDataset**: An usefull software that generates dataset states for model training from episodes keeped on server directory.  

## Source code
**RL/models/ai**: The agent IA, using torch.  
**RL/models/ai/model.ipynb**: Notebook with the research of the Actor Critic model on torch.  
**RL/models/prog**: Implements agentProg rules based.  

## Scripts
**RL/scripts/generateEpisodesToPreTraining.py**: It’s for generate a lot of episodes. Lunch serverMain, some serverWorld and a lot of agents, wait for agents and world to end. Repeat many times.  

**RL/scripts/generateEpisodesToTrainRL.py**: It’s for train the model on policy way. Lunch serverMain, some serverWorld, a lot of agents IA torch that act from the current model and execute model_exec.py that recive the state and return the action. After train the model with the updated data with model_train.py. Repeat many times.  

**RL/scripts/generateOnlyAgents.py**: Just lunch a lot of agents and wait for their ended.  

**RL/scripts/run.sh**: Just execute a command in a remote server and prevent that the ended when I exit the ssh.  

**RL/models/ai/model_exec.py**: Execute the model saved in ./saves/final/model.pth. Receive the states from /tmp/fifo_bw_agent_topython_%d and send action to /tmp/fifo_bw_agent_tocpp_%d.  

**RL/models/ai/model_train.py**: Train the model in an online way. Read state from /tmp/fifo_bw_topython and send commands to /tmp/fifo_bw_tocpp.  

## Generate dataset from episodes
From the episodes saves from server this command export them to a dataset easy to use for AI algorithms.  
The exported data is saved on dataset/episodes/  

For export the episodes  
```bash
cd dataset/
./makeDataset
```

Other options:  
```bash
makeDataset --net --dir <dir to save> --no-save --replays <dir> --use-noaction --use-only-icecool

--dir <dir to save>: Default ./episodes
--replays <dir>: Default ../server/data/replays
--net: Default false
--use-noaction: Default false
--use-only-icecool: Default false
--no-save: Not save to disk
```

**RL/models/episodes.zip**: There are some episodes data for training.






















