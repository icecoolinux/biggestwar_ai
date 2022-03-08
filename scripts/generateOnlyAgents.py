import os
import subprocess
import random
import time

AMOUNT_AGENTS = 45


# Lunch agents
agents_proc = []
for k in range(1,AMOUNT_AGENTS):
	#f = subprocess.DEVNULL
	f = open("../logs/log_"+str(k)+".txt", "w")
	agents_proc.append( subprocess.Popen(["./agentProg",  "AgentDummy"+str(k), "pass"], stdout=f, cwd="..") )
	time.sleep(1)
	
# Wait for worlds and agents.
for a in agents_proc:
	a.wait()
