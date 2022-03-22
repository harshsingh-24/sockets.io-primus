/* This is the final Client side code for chat application. -19CS01061*/

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore.h>
using namespace std;
#define MAX 1000

// color encoding
#define RED "\033[31m" 
#define RESET "\033[0m"
#define YELLOW "\033[33m"

pthread_t temp;

class client
{
	public:
	
	// Member variables
	int port, sockfd, connectid;
	struct hostent* server;
	struct sockaddr_in serv_addr;

	// Member Functions
	void getPort(char* argv[])
	{
		port = atoi(argv[2]);
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
	
	void getServer(char *argv[])
	{
		bzero(&serv_addr, sizeof(serv_addr));
		
		server = gethostbyname(argv[1]);
		if(server == NULL)
    	{ 
    		cout << "Error... Server IP is invalid" << endl;
        	exit(1);
    	}
    
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_port = htons(port);
    	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	}
	
	void connectClient()
	{
		connectid = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); 
		
		if(connectid < 0)
		{ 
			cout << "Connection with Server Failed... exiting" << endl;
			exit(0);
    	}
    	else
    		cout << "Connection Established with the Server" << endl;
	}
	
	string readServer(int a)
	{	
		// Read from the server 
		char ip[MAX];
		bzero(ip, sizeof(ip));
		
		read(a, ip, 8 * sizeof(ip));
		
		string ans(ip);
		return ans;
	}
	
	void writeServer(string s, int a)
	{
		// Write back to the server
		char *ptr = &s[0];
		write(a, ptr, 8 * sizeof(s));
	}

	void closeClient(int a)
	{
		close(sockfd);
	}
} c;

void signalHandler( int signum ) 
{
    c.writeServer("#CLOSE#", c.sockfd);
    exit(signum);  
}

string command(string s)
{
    stringstream ss(s);
    string item;
    char delim = ' ';

    getline(ss, item, delim);
    return item;
}

string decode(string s)
{
	stringstream ss(s);
	int j = 0;
	char delim = ' ';
	string item, ans = "";
    getline(ss, item, delim);

    //ans = ans + "\n(Server):\nClient-ID\tStatus\n";
	while(getline(ss, item, delim))
	{
		if(item == "0")
        {
            ans = ans + to_string(j);
            ans = ans + "\t\t FREE\n";
        }
		else if(item == "1")
		{
            ans = ans + to_string(j);
            ans = ans + "\t\t BUSY\n";
        }
		j++;
	}
    return ans;
}

void *readHandler(void *arg)
{
	while(1)
	{
		string s = c.readServer(c.sockfd);
        string com = command(s);

        if(com == "GET")
        {
            string ans = decode(s);
            cout << RED << "\t(Server):" << RESET << "\nClient-ID\tStatus" << endl;
            cout << ans << endl;
        }
        else if(com == "CONNECT")
        {
            if(s == "CONNECT OFFLINE")
            {
                cout << RED << "\t(Server): " << RESET << "Invalid. Offline." << endl;
				continue;
            }
			else if(s == "CONNECT BUSY")
			{
				cout << RED << "\t(Server): " << RESET << "Invalid. Other Client is Busy." << endl;
				continue;
			}
			else if(s == "CONNECT TALK")
			{
				cout << RED << "\t(Server): " << RESET << "Invalid. Other Client is talking to you." << endl;
				continue;
			}
			else if(s == "CONNECT SELF")
			{
				cout << RED << "\t(Server): " << RESET << "Invalid. Connection to self." << endl;
				continue;
			}
            else
            {
                stringstream temp(s);
                char delim = ' ';
                string item;

                getline(temp, item, delim);
                getline(temp, item, delim);

                if(item == "SUCCESS")
                {
                    getline(temp, item, delim);
                    cout << RED << "\t(Server): " << RESET << "Successfully Connected to Client-ID " << item << endl;
                }
                else 
                {
                    getline(temp, item, delim);
                    cout << RED << "\t(Server): " << RESET << "Request. You are now connected to Client-ID " << item << endl;
                    //cout << "(user): "; // Not working
                }
            }
        }
        else
        {
            if(s == "TERMINATED")
            {
                cout << RED << "\t(Server): " << RESET << "You are now disconnected from the other client. We hope it was a nice experience." << endl; 
                continue;
            }
            else if(s == "ABORT")
            {
                cout << RED << "\t(Server): " << RESET << "You are now disconnected from the Server. We hope you had a nice experience." << endl;
                pthread_cancel(temp);
                pthread_exit(NULL);
                exit(0);
                return NULL;
            }
            else if(s == "OUTAGE")
            {
                cout << RED << "\t(Server): " << RESET << "Disconnected. Server is offline. Sorry for the inconvenience." << endl;
                pthread_cancel(temp);
                pthread_exit(NULL);
                exit(0);
                return NULL;
            }
            else if(s == "INVALID COMMAND")
            {
                cout << RED << "\t(Server): " << RESET << "Invalid Command. Try the following - 1) GET 2) CONNECT <client_id>" << endl;
            }
            else
            {
                // Spit out everything
                cout << YELLOW << "\t(Friend): " << RESET << s.substr(4) << endl;
            }
        }
	}
    pthread_exit(NULL);
}

void *writeHandler(void *arg)
{
    temp = pthread_self();
	while(1)
	{
		string input;
        getline(cin, input);
		c.writeServer(input, c.sockfd);
        sleep(1);
	}
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		cout << "Error..Server IP or Port Number missing" << endl;
		exit(0);
	}
	
	// signal handling
    signal(SIGINT, signalHandler);

	c.getPort(argv);
	
	c.socketNumber();
	if(c.sockfd < 0)
		exit(0);
		
	c.getServer(argv);
	if(c.server == NULL)
		exit(0);
		
	c.connectClient();
	if(c.connectid < 0)
		exit(0);
	
    // Thread for reading and writing
	pthread_t read, write;
	pthread_create(&read, NULL, readHandler, NULL);
	pthread_create(&write, NULL, writeHandler, NULL);

	pthread_join(read, NULL);
	pthread_join(write, NULL);

	c.closeClient(c.sockfd);
}
