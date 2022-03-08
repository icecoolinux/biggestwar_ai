#ifndef _ClientMessage_h_
#define _ClientMessage_h_

class ClientMessage;

#include "../../../server/src/config.h"
#include "net.h"
#include "vec2.h"
#include <string.h>
#include <string>

using namespace std;

#define MAX_SELECTION 2000

class ClientMessage
{
	private:

	public:
		
		bool login_;
		char username_Login[LEN_NAME+1];
		char pass_Login[LEN_PASS+1];
		
		bool play_;
		
		bool loginGame_;
		char username_LoginGame[LEN_NAME+1];
		char token_LoginGame[LEN_TOKEN+1];
		
		bool surrender_;
		
		bool update_;
		int xMin_Update;
		int xMax_Update;
		int yMin_Update;
		int yMax_Update;

		bool intent_;
		int type_Intent;
		int typeType_Intent;
		unsigned long long unitId_Intent;
		unsigned long long unit2Id_Intent;
		unsigned long long buildId_Intent;
		unsigned long long build2Id_Intent;
		unsigned long long objectMapId_Intent;
		vec2 pos_Intent;
		int make_Intent;
		
		bool cancelAction_;
		unsigned long long objectId_CancelAction;
		
		bool miniMap_;
		int res_MiniMap;
		int zoom_MiniMap;
		vec2 posWorld_MiniMap;
		
		bool selection_;
		int cantSelection;
		unsigned long long idSelection[MAX_SELECTION];
		
	
		
		
		ClientMessage();
		~ClientMessage();
		
		void clear();
		
		void login(char* name, char* pass);
		void loginGame(char* user, char* token);
		void play();
		void surrender();
		void update(int xMin, int xMax, int yMin, int yMax);
		void intent(int type, int typeType, unsigned long long unitId, unsigned long long unit2Id, unsigned long long buildId, unsigned long long build2Id, unsigned long long objectMapId, vec2 pos, int make);
		void cancelAction(unsigned long long objectId);
		void miniMap(int res, int zoom, vec2 posWorld);
		
		
		
		int set(char* msgS, int len);
		string get();
};

#endif
