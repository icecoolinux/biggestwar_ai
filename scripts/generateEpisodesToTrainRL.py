import os
import subprocess
import random
import time

AMOUNT_LOOPS = 2
AMOUNT_WORLDS_EACH_LOOP = 1
AMOUNT_AGENTS_BY_WORLD_MIN = 3
AMOUNT_AGENTS_BY_WORLD_MAX = 9

# Ejecuto mainlogin
main_login_proc = subprocess.Popen(["./serverLogin", "8888"], cwd="../../server/")

for i in range(AMOUNT_LOOPS):
	print("Loop "+str(i)+"/"+str(AMOUNT_LOOPS))
	
	# Lunch worlds
	worlds_proc = []
	for j in range(AMOUNT_WORLDS_EACH_LOOP):
		worlds_proc.append( subprocess.Popen(["./serverWorld", "ws://127.0.0.1", str(8270+j)], cwd="../../server/") )
		time.sleep(2)
	
	# Lunch python model
	python_model = subprocess.Popen(["python", "model_exec.py"], cwd="../models/ai/")
	time.sleep(2)
	
	# Lunch agents
	agents_proc = []
	amount_agents = random.randint(AMOUNT_AGENTS_BY_WORLD_MIN, AMOUNT_AGENTS_BY_WORLD_MAX)
	amount_agents *= AMOUNT_WORLDS_EACH_LOOP
	for k in range(1,amount_agents+1):
		#f = open("./logs/log_"+str(k)+".txt", "w")
		f = subprocess.DEVNULL
		agents_proc.append( subprocess.Popen(["./agentIAtorch",  "AgentDummy"+str(k), "pass"], stdout=f, cwd="..") )
		time.sleep(0.2)
	#agents_proc.append( subprocess.Popen(["./agentProg",  "AgentDummy15", "pass"], stdout=subprocess.DEVNULL) )
	
	# Wait for worlds, agents and python model.
	for w in worlds_proc:
		w.wait()
	for a in agents_proc:
		a.wait()
	python_model.kill()
		
	# Remove fifos
	os.system("rm -f /tmp/fifo_bw_*")
	
	# Train
	makedataset_proc = subprocess.Popen(["./makeDataset", "--no-save", "--net"], cwd="../dataset/")
	time.sleep(2)
	train_proc = subprocess.Popen(["python", "model_train.py"], cwd="../models/ai/")
	time.sleep(2)
	
	# Wait for the train procs
	makedataset_proc.wait()
	train_proc.wait()
	
	# Move episodes to a directory
	os.system("mkdir ../models/ai/saves/episodes/"+str(i))
	os.system("mv ../../server/data/replays/* ../models/ai/saves/episodes/"+str(i)+"/")
	
	# Copy model each 10 loops
	if i%10 == 0:
		os.system("mkdir ../models/ai/saves/training/"+str(i))
		os.system("cp -R ../models/ai/saves/final/* ../models/ai/saves/training/"+str(i)+"/")
	

# Terminate the mainLogin
main_login_proc.kill()


# Train model sending it the episodes
#os.system('./dataset/makeDataset --no-save --net --replays ../server/data/replays')
#os.system('python ./models/ai/pretrain.py --net --no-from-disk')
