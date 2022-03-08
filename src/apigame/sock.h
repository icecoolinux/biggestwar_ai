#ifndef _sock_h_
#define _sock_h_

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <list>
#include "semaphore.h"
#include "time.h"


#include "../../../server/src/config.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::connection_hdl;
using namespace std;

class Sock
{
	private:
		
		// Manage the connections
		client endpoint;
		bool threadInited;
		websocketpp::lib::shared_ptr<websocketpp::lib::thread> manageThread;
		
		// Handles net low level
		static void on_open(Sock* sock, websocketpp::connection_hdl hdl);
		static void on_fail(Sock* sock, websocketpp::connection_hdl hdl);
		static void on_message(Sock* sock, websocketpp::connection_hdl hdl, message_ptr msg);
		static void on_close(Sock* sock, websocketpp::connection_hdl hdl);
		
		// About the current socket/connection
		string uri;
		connection_hdl hdl;
		list<string> messages;
		bool closed;
		bool connected;
		bool isError;
		
		Semaphore* sem;
		
	public:
		
		Sock();
		~Sock();
		
		bool connectToServer(string uri);
		
		connection_hdl getConnection();
		
		bool isConnected();
		bool isClosed();
		
		bool getMessage(string &msg);
		void setMessage(string msg);// Back for default
		void setMessageFront(string msg);
		
		bool send(string data);
		bool sendBinary(char* data, int size);
		
		bool close();
};

#endif

