import os
import subprocess
import random
import time

AMOUNT_LOOPS = 2
AMOUNT_WORLDS_EACH_LOOP = 2
AMOUNT_AGENTS_BY_WORLD_MIN = 5
AMOUNT_AGENTS_BY_WORLD_MAX = 10

# Ejecuto mainlogin
main_login_proc = subprocess.Popen(["../serverLogin", "8888"], cwd="../../server/")

for i in range(AMOUNT_LOOPS):
	print("Loop "+str(i)+"/"+str(AMOUNT_LOOPS))
	
	# Lunch worlds
	worlds_proc = []
	for j in range(AMOUNT_WORLDS_EACH_LOOP):
		worlds_proc.append( subprocess.Popen(["../serverWorld", "ws://127.0.0.1", str(8270+j)], cwd="../../server/") )
		time.sleep(2)
	
	# Lunch agents
	agents_proc = []
	amount_agents = random.randint(AMOUNT_AGENTS_BY_WORLD_MIN, AMOUNT_AGENTS_BY_WORLD_MAX)
	amount_agents *= AMOUNT_WORLDS_EACH_LOOP
	for k in range(1,amount_agents+1):
		f = open("../logs/log_"+str(k)+".txt", "w")
		#f = subprocess.DEVNULL
		agents_proc.append( subprocess.Popen(["../agentProg",  "AgentDummy"+str(k), "pass"], stdout=f, cwd="..") )
		time.sleep(0.2)
	#agents_proc.append( subprocess.Popen(["./agentProg",  "AgentDummy15", "pass"], stdout=subprocess.DEVNULL) )
	
	# Wait for worlds and agents.
	for w in worlds_proc:
		w.wait()
	for a in agents_proc:
		a.wait()

# Terminate the mainLogin
main_login_proc.kill()


# Train model sending it the episodes
#os.system('./models/v6/makeDataset --no-save --net --replays ../server/dev/data/replays')
#os.system('python ./models/v7/pretrain.py --net --no-from-disk')
