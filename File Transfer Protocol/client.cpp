// This is the final program for File Transfer Protocol - Harsh Singh Jadon

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

using namespace std;
#define MAX 1000
#define SIZE 1024

// color encoding
#define RED "\033[31m" 
#define RESET "\033[0m"
#define YELLOW "\033[33m"

// file constants
#define FILENAME "//toc/foo.c"

pthread_t w, r;

class client
{
	public:
	
	// Member variables
	int port, sockfd, connectid, authid;
    string username;
	struct hostent* server;
	struct sockaddr_in serv_addr;

	// Member Functions
    client()
    {
        authid = -2;
    }

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
    		cout << "Connection Established with the FTP Server" << endl;
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

void printProgress(double percentage) 
{
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

void drawLine()
{
    cout << string(50, '-') << endl;
}

string authClient()
{
    string ans;
    while(1)
    {
        string choice, username, password;
        ans = "AUTH ";
        cout << "Are you a new user? " << endl;
        cout << " Y: Yes (New user gets registered)" << endl;
        cout << " N: No (Already Existing)" << endl;
        cout << " Q: Quitting" << endl << endl;        
        cout << endl;

        cout << "Enter your choice: ";
        cin >> choice;
        ans = ans + choice + " ";
        if(choice == "Y")
        {
            cout << "Enter new username: ";
            cin >>  username;

            c.username = username;

            cout << "Enter new password: ";
            cin >> password;

            ans = ans + username + " ";
            ans = ans + password;
            c.authid = 1;
            break;
        }
        else if(choice == "N") 
        {
            cout << "Enter username: ";
            cin >>  username;
            cout << "Enter password: ";
            cin >> password;

            ans = ans + username + " ";
            ans = ans + password;
            c.authid = 1;
            break;
        }
        else if(choice == "Q")
        {
            return "QUIT";
        }
        else
        {
            cout << "Invalid choice. You cannot skip authentication..." << endl;
            continue;
        }
    }
    
    return ans;
}

vector<string> command(string s)
{
    vector<string> ans;
    stringstream ss(s);
	char delim = ' ';
	string item;

	while(getline(ss, item, delim))
		ans.push_back(item);
	
	return ans;
}

void showOptions()
{
    cout << "The following are the allowed operations for file transfer: " << endl;
    cout << "1. GET <mode> <filename>" << endl;
    cout << "2. POST <mode> <filename>" << endl;
    cout << "3. SHOW SELF       - (In order to list out the files in client directory)" << endl;
    cout << "4. SHOW SERVER     - (In order to list out the files of the client in server directory)" << endl;
    cout << "5. DELETE SELF <filename>" << endl;
    cout << "6. DELETE SERVER <filename>" << endl;
    cout << "7. #CLOSE#" << endl;
    cout << "Possible <modes> are: 0  - BINARY OR 1 - ASCII" << endl;
    drawLine();
}

void readMetadata()
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./client/storage/metadata.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1) 
    {
        printf("%s", line);
    }

    fclose(fp);
    if (line)
        free(line);
    
    return;
}   

void deleteFile(string path)
{
    char *ptr = &path[0];

    if (remove(ptr) == 0)
    {
        printf("Deleted file succesfully from client.\n");
    }
    else 
    {
        printf("Error. File does not exist on client storage.\n");
    }
}

void *readHandler(void *arg)
{
    int size, meta = 0;
    r = pthread_self();
	while(1)
	{
		string s = c.readServer(c.sockfd);
        vector<string> comm = command(s);
        //cout << s << endl;

        if(comm[0] == "AUTH")  // Authorisation is completed.
        {
            if(s == "AUTH NEWPASS")
            {
                cout << "New user created." << endl;

                drawLine();
                showOptions();
                continue;
            }
            else if(s == "AUTH PASS")
            {
                cout << "User has been authenticated successfully." << endl;
                drawLine();
                showOptions();
                continue;
            }
            else if(s == "AUTH FAIL")
            {
                cout << "Invalid username or password. Not allowed." << endl;
                c.writeServer("#CLOSEAUTH#", c.sockfd);
                pthread_cancel(w);
                pthread_exit(NULL);
                exit(0);
                return NULL;
            }
        }
        else if(comm[0] == "POST")
        {
            if(comm[1] == "0")
            {
                string input = "";
                // post files in zero/binary mode
                struct stat st;

                string directory = "./client/storage/" + comm[3];
                cout << "Location of Source File: " << directory << endl;
                char *filename = &directory[0];

                if(stat(filename, &st) == -1)
                {
                    c.writeServer("POST ERROR", c.sockfd);
                    cout << "Error. File does not exist." << endl;
                    drawLine();
                    continue;
                }

                off_t size = st.st_size; // total size in bytes
                input = input + "POST 0 " +  comm[3] + " ";
                input = input + to_string(size);

                c.writeServer(input, c.sockfd);
                sleep(2);

                // Let us send the actual file to the server - In binary mode
                // 1. open the file
                FILE * fp;
                if (fp = fopen(filename, "r")) 
                    cout << "Source file opened successfully" << endl;
                else
                {
                    cout << "Error in source file opening" << endl;
                    // Do something
                }
                
                int sent = 0;
                char buffer[1024] = {0};
                int remain_data = size;

                // 2. send the file
                while(1)
                {
                    if(remain_data <= 0)
                    {
                        cout << endl << "Sending of file completed (from client to server)." << endl;
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        fread(buffer, sizeof(char), remain_data, fp);
                        int bytes = send(c.sockfd, buffer, remain_data, 0);
                        sent += bytes;
                        printProgress((double)sent/size);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    fread(buffer, sizeof(char), 1024, fp);
                    int bytes = send(c.sockfd, buffer, 1024, 0);
                    sent += bytes;
                    printProgress((double)sent/size);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(fp);
            }
            else if(comm[1] == "1")
            {
                string input = "";
                // post files in ascii mode
                struct stat st;

                string directory = "./client/storage/" + comm[3];
                cout << "Location of Source File: " << directory << endl;
                char *filename = &directory[0];

                if(stat(filename, &st) == -1)
                {
                    c.writeServer("POST ERROR", c.sockfd);
                    cout << "Error. File does not exist." << endl;
                    drawLine();
                    continue;
                }

                off_t size = st.st_size; // total size in bytes
                input = input + "POST 1 " +  comm[3] + " ";
                input = input + to_string(size);

                c.writeServer(input, c.sockfd);
                sleep(2);

                // Let us send the actual file to the server - In binary mode
                // 1. open the file
                FILE * fp;
                if (fp = fopen(filename, "rb")) 
                    cout << "Source file opened successfully" << endl;
                else
                {
                    cout << "Error in source file opening" << endl;
                    // Do something
                }
                
                int sent = 0;
                char buffer[1024] = {0};
                int remain_data = size;

                // 2. send the file
                while(1)
                {
                    if(remain_data <= 0)
                    {
                        cout << endl << "Sending of file completed (from client to server)." << endl;
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        fread(buffer, sizeof(char), remain_data, fp);
                        int bytes = send(c.sockfd, buffer, remain_data, 0);
                        sent += bytes;
                        printProgress((double)sent/size);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    fread(buffer, sizeof(char), 1024, fp);
                    int bytes = send(c.sockfd, buffer, 1024, 0);
                    sent += bytes;
                    printProgress((double)sent/size);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(fp);
            }
            else if(comm[1] == "SUCCESS")
            {
                cout << "OK from server. File transfer completed." << endl;
                drawLine();
            }
        }
        else if(comm[0] == "GET")
        {
            if(s == "GET ERROR")
            {
                cout << "File does not exist on the Server. Terminating request..." << endl;
                drawLine();
                continue;
            }

            cout << "Mode: " << comm[1] << "\nFilename: " << comm[2] << "\nSize: " << comm[3] << "  bytes" << endl;

            if(comm[1] == "0")
            {
                string overall = "./client/storage/" + comm[2];
                cout << "Destination Directory: " << overall << endl;

                // This is mode=0 (binary) receiving
                char *fp = &overall[0];   
                FILE *file;

                if(file = fopen(fp, "w"))
                    cout << "Destination file opened successfully" << endl;
                else
                    cout << "Error in destination file opening" << endl;

                // Receive the file from server
                int sent = 0, size = stoi(comm[3]);
                char buffer[1024];
                int remain_data = stoi(comm[3]);
                ssize_t len;

                while(1) 
                {
                    if(remain_data <= 0)
                    {
                        cout << endl << "Receiving of file completed. (from server to client)" << endl;
                        drawLine();
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        int bytes = recv(c.sockfd, buffer, remain_data, 0);
                        fwrite(buffer, sizeof(char), bytes, file);
                        sent += bytes;
                        printProgress((double)sent/size);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    int bytes = recv(c.sockfd, buffer, 1024, 0);
                    fwrite(buffer, sizeof(char), bytes, file);
                    sent += bytes;
                    printProgress((double)sent/size);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(file);

                sleep(2);

                if(meta == 1)
                {
                    meta = 0;
                    readMetadata();
                    drawLine();
                    continue;
                }
                c.writeServer("GET SUCCESS", c.sockfd);
            }
            else if(comm[1] == "1")
            {
                string overall = "./client/storage/" + comm[2];
                cout << "Destination Directory: " << overall << endl;

                // This is mode=0 (binary) receiving
                char *fp = &overall[0];   
                FILE *file;

                if(file = fopen(fp, "wb"))
                    cout << "Destination file opened successfully" << endl;
                else
                    cout << "Error in destination file opening" << endl;

                // Receive the file from server
                int sent = 0, size = stoi(comm[3]);
                char buffer[1024];
                int remain_data = stoi(comm[3]);
                ssize_t len;

                while(1) 
                {
                    if(remain_data <= 0)
                    {
                        cout << endl << "Receiving of file completed. (from server to client)" << endl;
                        drawLine();
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        int bytes = recv(c.sockfd, buffer, remain_data, 0);
                        fwrite(buffer, sizeof(char), bytes, file);
                        sent += bytes;
                        printProgress((double)sent/size);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    int bytes = recv(c.sockfd, buffer, 1024, 0);
                    fwrite(buffer, sizeof(char), bytes, file);
                    sent += bytes;
                    printProgress((double)sent/size);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(file);

                sleep(2);

                if(meta == 1)
                {
                    meta = 0;
                    readMetadata();
                    drawLine();
                    continue;
                }
                c.writeServer("GET SUCCESS", c.sockfd);
            }
        }
        else if(comm[0] == "DELETE")
        {
            if(comm[1] == "SELF")
            {
                deleteFile("./client/storage/" + comm[2]);
                drawLine();
            }
            else
            {
                string s = c.readServer(c.sockfd);
                if(s == "1")
                    cout << "File deleted successfully" << endl;
                else
                    cout << "File does not exist." << endl;
                drawLine();
                continue;
            }
        }
        else if(s == "SHOW SELF")
        {
            string directory = "./client/storage/";
            char *dir = &directory[0];
            char cmd[1000];
            int result = 0;

            sprintf(cmd, "ls %s", dir);
            result = system(cmd);

            if (result == 0)
                printf("File contents shown.\n");
            else
                printf("Contents not shown.\n");

            drawLine();
        }
        else if(s == "SHOW SERVER")
        {
            if(meta == 0)
            {
                meta = 1;
                c.writeServer("GET 0 metadata.txt", c.sockfd);
                continue;
            }
        }
        else 
        {
            if(s == "ABORT")
            {
                cout << "You are now disconnected from the Server. We hope you had a nice experience." << endl;
                pthread_cancel(w);
                pthread_exit(NULL);
                exit(0);
                return NULL;
            }
            else if(s == "INVALID COMMAND")
            {
                cout << "Invalid Command." << endl;
                drawLine();
                continue;
            }
        }
	}
    pthread_exit(NULL);
}

void *writeHandler(void *arg)
{
    int flag = 0;
    w = pthread_self();
	while(1)
	{
        if(flag == 0)
        {
            flag = 1;
            string final = authClient();
            if(final == "QUIT")
            {
                c.writeServer("#CLOSEAUTH#", c.sockfd);
                cout << "Quitting the program." << endl;
                pthread_cancel(r);
                pthread_exit(NULL);
                exit(0);
                return NULL;
            }
            c.writeServer(final, c.sockfd);
            continue;
        }
        
		string input;
        getline(cin, input);
		c.writeServer(input, c.sockfd);
        fflush(stdin);
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

	c.getPort(argv);
	
	c.socketNumber();
	if(c.sockfd < 0)
		exit(0);
		
	c.getServer(argv);
	if(c.server == NULL)
		exit(0);
	
    // Before connecting to the server, we must authenticate the client as well.  [Will do it later]

	c.connectClient();
	if(c.connectid < 0)
		exit(0);
    
    // After connection we should give the options to the user
    // Thread for reading and writing
	pthread_t read, write;
    sleep(1);
	pthread_create(&read, NULL, readHandler, NULL);
	pthread_create(&write, NULL, writeHandler, NULL);

	pthread_join(read, NULL);
	pthread_join(write, NULL);

	c.closeClient(c.sockfd);

}