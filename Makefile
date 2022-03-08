
#INCPATH := $(shell python -c "import torch.utils.cpp_extension as C; print('-I' + str.join(' -I', C.include_paths()))")
#LIBPATH := $(shell python -c "import torch.utils.cpp_extension as C; print(C.include_paths()[0] + '/../')")
#USE_CUDA := $(shell python -c "import torch; print(torch.cuda.is_available())")

todo: agentIAtorch agentProg dataset/makeDataset

dataset/makeDataset: obj/makeDataset.o obj/ServerMessage.o obj/ClientMessage.o obj/vec2.o obj/time.o obj/inout.o obj/net.o obj/sock.o obj/semaphore.o obj/img.o
	g++ -g obj/makeDataset.o obj/ServerMessage.o obj/ClientMessage.o obj/vec2.o obj/time.o obj/inout.o obj/net.o obj/sock.o obj/semaphore.o obj/img.o -o dataset/makeDataset -lpthread -lboost_system -lstdc++fs `pkg-config --libs opencv4`
agentIAtorch: obj/main_iatorch.o obj/agent_ia_torch.o obj/inout.o obj/net.o obj/vec2.o obj/ServerMessage.o obj/ClientMessage.o obj/semaphore.o obj/time.o obj/sock.o obj/img.o obj/apigame.o
	g++ -g obj/main_iatorch.o obj/agent_ia_torch.o obj/inout.o obj/net.o obj/ServerMessage.o obj/ClientMessage.o obj/vec2.o obj/semaphore.o obj/time.o obj/sock.o obj/img.o obj/apigame.o -o agentIAtorch -lpthread -lboost_system -L/usr/lib/ -lprotobuf  `pkg-config --libs opencv4`
agentProg: obj/main_prog.o obj/agent_prog.o obj/inout_prog.o obj/net.o obj/vec2.o obj/ServerMessage.o obj/ClientMessage.o obj/semaphore.o obj/time.o obj/sock.o obj/img.o obj/apigame.o
	g++ -g obj/main_prog.o obj/agent_prog.o obj/inout_prog.o obj/net.o obj/ServerMessage.o obj/ClientMessage.o obj/vec2.o obj/semaphore.o obj/time.o obj/sock.o obj/img.o obj/apigame.o -o agentProg -lpthread -lboost_system -L/usr/lib/ -lprotobuf `pkg-config --libs opencv4`

obj/main_prog.o: src/main.cpp
	g++ -g -c src/main.cpp -o obj/main_prog.o -DPROG -I/usr/include/opencv4
obj/main_ia.o: src/main.cpp
	g++ -g -c src/main.cpp -o obj/main_ia.o -DIA -I/usr/local/include/tensorflow/bazel-bin/bin/tensorflow/include/ -I/usr/local/include/tensorflow/bazel-bin/bin/tensorflow/include/src/
obj/main_iatorch.o: src/main.cpp
	g++ -g -c src/main.cpp -o obj/main_iatorch.o -DIATORCH -I/usr/include/opencv4

	
	
# Model Prog
obj/agent_prog.o: models/prog/agent.cpp models/prog/agent.h
	g++ -g -c models/prog/agent.cpp -o obj/agent_prog.o -I/usr/include/opencv4
obj/inout_prog.o: models/prog/inout.h models/prog/inout.cpp
	g++ -g -c models/prog/inout.cpp -o obj/inout_prog.o -I/usr/include/opencv4
	
	
# Dataset
obj/makeDataset.o: dataset/makeDataset.cpp
	g++ -g -c dataset/makeDataset.cpp -o obj/makeDataset.o -I/usr/include/opencv4


# Model AI torch
obj/agent_ia_torch.o: models/ai/agent.cpp models/ai/agent.h
	g++ -g -c  models/ai/agent.cpp -o obj/agent_ia_torch.o -I/usr/include/opencv4
#obj/agent_ia_torch.o: models/ai/agent_torch.cpp models/ai/agent_torch.h
#	g++ -g -c  models/ai/agent_torch.cpp -o obj/agent_ia_torch.o -I./libtorch/include -L./libtorch/lib
	

	
# Common source code
obj/apigame.o: src/apigame/api.h src/apigame/api.cpp
	g++ -g -c src/apigame/api.cpp -o obj/apigame.o
	
obj/inout.o: src/apigame/inout.h src/apigame/inout.cpp
	g++ -g -c src/apigame/inout.cpp -o obj/inout.o -I/usr/include/opencv4
	
obj/net.o: src/apigame/net.cpp src/apigame/net.h
	g++ -g -c src/apigame/net.cpp -o obj/net.o

obj/ServerMessage.o: src/apigame/ServerMessage.cpp src/apigame/ServerMessage.h
	g++ -g -c src/apigame/ServerMessage.cpp -o obj/ServerMessage.o

obj/ClientMessage.o: src/apigame/ClientMessage.cpp src/apigame/ClientMessage.h
	g++ -g -c src/apigame/ClientMessage.cpp -o obj/ClientMessage.o

obj/img.o: src/apigame/img.h src/apigame/img.cpp
	g++ -g -c src/apigame/img.cpp -o obj/img.o -I/usr/include/opencv4
	
obj/vec2.o: src/apigame/vec2.h src/apigame/vec2.cpp
	g++ -g -c src/apigame/vec2.cpp -o obj/vec2.o
	
obj/semaphore.o: src/apigame/semaphore.h src/apigame/semaphore.cpp
	g++ -g -c src/apigame/semaphore.cpp -o obj/semaphore.o

obj/time.o: src/apigame/time.h src/apigame/time.cpp
	g++ -g -c src/apigame/time.cpp -o obj/time.o

obj/sock.o: src/apigame/sock.h src/apigame/sock.cpp
	g++ -g -c src/apigame/sock.cpp -o obj/sock.o
	
clean:
	rm -rf obj/* agentIAtorch agentProg dataset/makeDataset



