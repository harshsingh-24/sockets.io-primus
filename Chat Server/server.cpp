/* This is the final Server side code for chat application. - 19CS01061*/

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
using namespace std;
#define MAX 1000

queue<int> clients;

int status[MAX];
int partner[MAX];
pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

class server
{
    public:
	
	// Member variables
	int port, sockfd, connectid, bindid, listenid, connfd;
	struct sockaddr_in serv_addr, cli_addr;
	
	// Constructor
	server()
	{
		memset(status, -1, sizeof(status));
		memset(partner, -1, sizeof(partner));
	}

	// Member Functions
	void getPort(char* argv[])
	{
		port = atoi(argv[1]);
	}
	
	void socketNumber()
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) 
    	{
    		cout << "Client Socket Creation Failed..exiting" << endl;
    		exit(0);
    	}
    	else
    		cout << "Client Socket was successfully created" << endl;
	}

    void socketBind()
    {
        bzero((sockaddr_in *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET ;
        serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        
        bindid = bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        if(bindid < 0)
        { 
            cout << "server socket bind failed." << endl;
            exit(0);
        }
        else 
            cout << "Server binded successfully." << endl;
    }

    void serverListen()
    {
        listenid = listen(sockfd, 20);
        if(listenid != 0)
        {
            cout << "server listen failed." << endl;
            exit(0);
        }
        else 
            cout << "server is listening." << endl;
    }

    void acceptClient()
    {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (connfd < 0) 
    	{
        	printf("server accept failed...\n");
        	exit(0);
    	}
    	else
        	printf("server-client connection established. !!\n");
    }

    string readClient(int a)
	{	
		// Read from the server 
		char ip[MAX];
		bzero(ip, sizeof(ip));
		
		read(a, ip, 8 * sizeof(ip));
		
		string ans(ip);
		return ans;
	}

    void writeClient(string s, int a)
	{
		// Write back to the server
		char *ptr = &s[0];
		write(a, ptr, 8 * sizeof(s));
	}

    void closeServer(int a)
    {
        close(a);
    }
} s; 

void sHandler(int signum) 
{
	for(int i=0; i<MAX; i++)
	{
		if(status[i] != -1)
		{
			s.writeClient("OUTAGE", i);
		}
	}
    exit(signum);  
}

string encoding(int connfd)
{
	string encode = "";
	for(int i=0; i<MAX; i++)
	{
		if(status[i] == 0)
			encode = encode + "0 "; // FREE
		else if(status[i] == 1)
			encode = encode + "1 "; // BUSY
		else if(status[i] == -1)
			encode = encode + "-1 "; // DISCONNECTED
	}
	return encode;
}

void logs(string s, int connfd)
{
	cout << "Client ID " << connfd << " requested for " << s << " command" << endl;
}

string parseMessage(string s)
{
	stringstream ss(s);
	char delim = ' ';
	string item;

	getline(ss, item, delim);
	string ans = "";
	while(getline(ss, item, delim))
		ans = ans + item;
	
	return ans;
}

void *handler(void *arg)
{
	pthread_mutex_lock(&qlock);
	int connfd = clients.front();
	clients.pop();
	pthread_mutex_unlock(&qlock);
	
	int flag = 0;
	
	while(1)
	{
		if(flag == 0)
		{
			cout << "Client ID " << connfd << " joined" << endl;
			cout << string(50, '-') << endl;
			flag = 1;
			status[connfd] = 0; // FREE and connected to the server
		}
		

        string client = s.readClient(connfd);

		pthread_mutex_lock(&mylock);
		cout << "Handler Locked using Mutex" << endl;
		if(client.compare("#CLOSE#") == 0)
		{
			s.writeClient("ABORT", connfd);
			cout << "Client ID " << connfd <<" exits" << endl;
			
			// When does the disconnection happens ?

			if(status[connfd] == 1)
			{
				// Disconnect the client (Critical Section?)
				int connfd2 = partner[connfd];
				status[connfd] = -1;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;
				s.writeClient("TERMINATED", connfd2);
			}
			else 
			{
				status[connfd] = -1; // Disconnected from the server
				partner[connfd] = -1;
			}
			s.closeServer(connfd);
			cout << "Handler unlocked using Mutex" << endl;
			cout << string(50, '-') << endl;
			pthread_mutex_unlock(&mylock);
			pthread_exit(NULL);
			return NULL;
		}

		if(status[connfd] == 1)
		{
			if(client == "#GOODBYE#")
			{
				// Disconnect the client (Critical Section?)
				
				int connfd2 = partner[connfd];
				cout << "Client ID " << connfd << " and Client ID " << connfd2 << " are now disconnected" << endl;
				status[connfd] = 0;
				status[connfd2] = 0;
				partner[connfd2] = -1;
				partner[connfd] = -1;

				s.writeClient("TERMINATED", connfd);
				s.writeClient("TERMINATED", connfd2);
			}
			else 
			{
				logs("SEND MESSAGE", connfd);
				s.writeClient("SEND " + client, partner[connfd]);
			}
			cout << "Handler unlocked using Mutex" << endl;
			cout << string(50, '-') << endl;
			pthread_mutex_unlock(&mylock);
			continue;
		}

		// Parsing of the string for command - GET, CONNECT, SEND
		vector<string> process;
		stringstream ss(client);
		char delim = ' ';
		string item, output, command;

		getline(ss, item, delim);
		command = item;

		if(command == "GET")
		{
			// Format: GET
			logs(command, connfd);

			// Encoding of the message from server to client
			string encode = encoding(connfd);
			s.writeClient("GET " + encode, connfd);
		}
		else if(command == "CONNECT")
		{
			// Format: CONNECT <partner id>
			logs(command, connfd);

			getline(ss, item, delim);
			int connfd2 = stoi(item);
			
			if(status[connfd2] == -1)
				s.writeClient("CONNECT OFFLINE", connfd);
			else if(connfd2 == connfd)
				s.writeClient("CONNECT SELF", connfd);
			else if(status[connfd2] == 1 && partner[connfd2] != connfd)
				s.writeClient("CONNECT BUSY", connfd);
			else if(status[connfd2] == 1 && partner[connfd2] == connfd)
				s.writeClient("CONNECT TALK", connfd);
			else
			{
				// Make the connection between the two (Critical Section ?)
				// This means that I have to inform the other client and make it listen
				status[connfd] = 1;
				status[connfd2] = 1;
				partner[connfd] = connfd2;
				partner[connfd2] = connfd;

				cout << "Client " << connfd << " and Client " << connfd2 << " are connected" << endl;

				string a = "CONNECT SUCCESS " + to_string(connfd2);
				string b = "CONNECT SUCCESS% " + to_string(connfd);
				s.writeClient(a, connfd);
				s.writeClient(b, connfd2);
			}
		}
		else
		{
			// Invalid command
			s.writeClient("INVALID COMMAND", connfd);
		}
		cout << "Handler unlocked using Mutex" << endl;
		cout << string(50, '-') << endl;
		pthread_mutex_unlock(&mylock);

	}
	s.closeServer(connfd);
	pthread_exit(NULL);
} 

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        cout << "Error. Port Number is missing." << endl;
        exit(0);
    } 

	// signal handling
    signal(SIGINT, sHandler);

    s.getPort(argv);
    s.socketNumber();
    if(s.sockfd < 0)
	    exit(0);   
    s.socketBind();
    if(s.bindid < 0)
        exit(0);
    s.serverListen();
    if(s.listenid != 0)
        exit(0);
	cout << string(50, '-') << endl;

    while(1)
    {
    	// accepting the client
        s.acceptClient();

    	if (s.connfd < 0) 
        	continue;
        
        clients.push(s.connfd);
    	pthread_t t; 
    	pthread_create(&t, NULL, handler, NULL);
    }

    s.closeServer(s.sockfd);
}