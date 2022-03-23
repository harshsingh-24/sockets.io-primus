// This is the final program for File Transfer Protocol - Harsh Singh Jadon

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
#include <sys/stat.h>
#include <dirent.h>
using namespace std;
#define MAX 1024

// Data structures
queue<int> clients;
map<string, string> m;

int status[MAX];
string usernames[MAX];

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
        for(int i=0; i<MAX; i++)
            usernames[i] = "";
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

void drawLine()
{
    cout << string(50, '-') << endl;
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

void initMap()
{
    // Load the map with file contents
    cout << "Loading the Map with users." << endl;
    m.clear();

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./server/auth/users.txt", "a+");
    if (fp == NULL)
    {
        cout << "Error in UserAuth file opening" << endl;
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp)) != -1) 
    {        
        char *token = strtok(line, "%%%%");
        string username(token);
        token = strtok(NULL, "%%%%");
        string password(token);
        m[username] = password;
    }

    fclose(fp);
    if (line)
        free(line);
    cout << "Users loaded into map successfully." << endl;
    drawLine();
}

void setMap()
{
    cout << "Setting Map started" << endl;
    FILE *file;
    if(file = fopen("./server/auth/users.txt", "w"))
        cout << "Userauth file opened successfully" << endl;
    else
        cout << "Error in UserAuth file opening" << endl;

    for(auto i: m)
    {   
        string final = "";
        final = final + i.first + "%%%%";
        final = final + i.second + "%%%%";
        final = final + "\n";

        char *ptr = &final[0];
        fprintf(file,"%s",ptr);
    }

    fclose(file);
    cout << "Setting Map completed." << endl;
    drawLine();
}

string makeDirectory(int a)
{
    // Create a new folder for the joined user
    string directory = "./server/storage/" + usernames[a] + "/";
    cout << directory << endl;
    char *filename = &directory[0];

    int res = mkdir(filename, 0700);
    if(res != -1)
        cout << "Storage directory for user made succesfully" << endl;
    else
        cout << "Storage directory for user already exists." << endl;
    
    drawLine();
    return directory;
}

void deleteDirectory(string path)
{
    // directory should be empty
    char *dir = &path[0];
    char cmd[1000];
    int result = 0;

    sprintf(cmd, "rmdir %s", dir);
    result = system(cmd);

    if (result == 0)
        printf("Directory delete completed.\n");
    else
        printf("Delete failed for the directory.\n");
    
    return;
}

string deleteFile(string path)
{
    char *ptr = &path[0];

    if (remove(ptr) == 0)
    {
        printf("Deleted file succesfully from server.\n");
        return "1";
    }
    else 
    {
        printf("Error. File does not exist on server\n");
        return "0";
    }
    return "";
}

void updateMetadata(string path)
{
    DIR *d;
    struct dirent *dir;
    char *p = &path[0];
    d = opendir(p);

    string overall = path + "metadata.txt";
    if(d)
    {
        cout << "Directory opened successfully." << endl;
        char *fp = &overall[0];   
        FILE *file;
        if(file = fopen(fp, "w"))
            cout << "Metdata File opened in write mode" << endl;
        else
            cout << "Error in opening metdata file in write mode" << endl;
        
        while ((dir = readdir(d)) != NULL)
        {
             if (dir->d_type == DT_REG)
                fprintf(file, "%s\n", dir->d_name);
        }
        fclose(file);
        closedir(d);
        cout << "Directory closed successfully" << endl;
    }

    return;
}

void showLogs(string s, int id)
{
    cout << "Client ID: " << id <<  " requested for " << s << " command" << endl;
    return;
}

void *handler(void *arg)
{
    pthread_mutex_lock(&qlock);
    int connfd = clients.front();
	clients.pop();
    pthread_mutex_unlock(&qlock);

	int flag = 0, postflag = 0, n, dirflag = 0;
    char buffer[MAX];
    string directory;
	
	while(1)
	{
		if(flag == 0)
		{
			cout << "Client ID " << connfd << " joined" << endl;
            status[connfd] = 1; // Connected to the server

			flag = 1;
            drawLine();
		}

        // After connection has been established from the server
        string client = s.readClient(connfd);

        pthread_mutex_lock(&mylock);
        cout << "Handler locked using Mutex" << endl;
        //cout << client << endl;

        if(client.compare("#CLOSE#") == 0)
        {
        	s.writeClient("ABORT", connfd);
        	cout << "Username: " << usernames[connfd] <<" exits" << endl;
            cout << "Client ID: " << connfd <<" exits" << endl;
            drawLine();
        	status[connfd] = -1;
            usernames[connfd] = "";

        	s.closeServer(connfd);
            cout << "Handler Unlocked using Mutex" << endl;
            pthread_mutex_unlock(&mylock);
        	pthread_exit(NULL);
        	return NULL;
        }
        if(client.compare("#CLOSEAUTH#") == 0)
        {
        	cout << "User Authentication Failed... Request to terminate !!" << endl;
            cout << "Client ID: " << connfd <<" exits" << endl;
            drawLine();
        	status[connfd] = -1;
            usernames[connfd] = "";

        	s.closeServer(connfd);
            cout << "Handler Unlocked using Mutex" << endl;
            pthread_mutex_unlock(&mylock);
        	pthread_exit(NULL);
        	return NULL;
        }

        vector<string> comm = command(client);

        if(comm[0] == "AUTH")
        {
            showLogs("AUTH", connfd);
            usernames[connfd] = comm[2];
            initMap();

            if(dirflag == 0)
            {
                directory = makeDirectory(connfd);
                dirflag = 1;
            }

            if(comm[1] == "Y")
            {
                m[comm[2]] = comm[3];
                s.writeClient("AUTH NEWPASS", connfd); 
                setMap();
            }
            else if(comm[1] == "N")
            {
                if(m[comm[2]] == comm[3])
                    s.writeClient("AUTH PASS", connfd);
                else
                {
                    deleteDirectory(directory);
                    s.writeClient("AUTH FAIL", connfd);
                }
            }
            else
            {
                s.writeClient("AUTH FAIL", connfd);
            }
        }
        else if(comm[0] == "POST")
        {
            showLogs("POST", connfd);
            if(client == "POST ERROR")
            {
                cout << "Error in posting the file to the server. Request Terminated." << endl;
                postflag = 0;
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                drawLine();
                continue;
            }
            if(postflag == 0)
            {
                string z = "POST " + comm[1];
                z = z + " SIZE " + comm[2];
                cout << z << endl;

                s.writeClient(z, connfd);
                postflag = 1;
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                sleep(2);
                continue;
            }

            int mode = stoi(comm[1]);
            string path = comm[2];
            int size = stoi(comm[3]);

            cout << "Mode of Transfer: " << mode << endl;
            cout << "Name of file(in Destination folder): " << path << endl;
            cout << "Size of File: " << size << endl;

            if(mode == 0)
            {
                string overall = directory + path;
                cout << "Destination: " << overall << endl;

                // This is mode=0 (binary) transfer
                char *fp = &overall[0];   
                FILE *file;
                if(file = fopen(fp, "w"))
                    cout << "File opened in write mode" << endl;
                else
                    cout << "Error in opening file in write mode" << endl;

                // Receive the file from client

                char buffer[1024];
                int remain_data = size;
                ssize_t len;

                while(1) 
                {
                    if(remain_data <= 0)
                    {
                        cout << "File receiving completed." << endl;
                        drawLine();
                        break;
                    }

                    if(remain_data < 1024)
                    {
                        int bytes = recv(connfd, buffer, remain_data, 0);
                        fwrite(buffer, sizeof(char), bytes, file);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    int bytes = recv(connfd, buffer, 1024, 0);
                    fwrite(buffer, sizeof(char), bytes, file);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }

                s.writeClient("POST SUCCESS", connfd);

                postflag = 0;
                fclose(file);
            }
            else if(mode == 1)
            {
                string overall = directory + path;
                cout << "Destination: " << overall << endl;

                // This is mode=0 (binary) transfer
                char *fp = &overall[0];   
                FILE *file;
                if(file = fopen(fp, "wb"))
                    cout << "File opened in write mode" << endl;
                else
                    cout << "Error in opening file in write mode" << endl;

                // Receive the file from client

                char buffer[1024];
                int remain_data = size;
                ssize_t len;

                while(1) 
                {
                    if(remain_data <= 0)
                    {
                        cout << "File receiving completed." << endl;
                        drawLine();
                        break;
                    }

                    if(remain_data < 1024)
                    {
                        int bytes = recv(connfd, buffer, remain_data, 0);
                        fwrite(buffer, sizeof(char), bytes, file);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    int bytes = recv(connfd, buffer, 1024, 0);
                    fwrite(buffer, sizeof(char), bytes, file);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }

                s.writeClient("POST SUCCESS", connfd);

                postflag = 0;
                fclose(file);
            }
        }
        else if(comm[0] == "GET")
        {
            showLogs("GET", connfd);
            if(client == "GET SUCCESS")
            {
                cout << "File transfer completed from server to client" << endl;
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                drawLine();
                continue;
            }
            
            if(comm[1] == "0")
            {
                string input = "";
                struct stat st;

                // file path inside server
                string path = directory + comm[2];
                cout << "Source Directory of file: " << path << endl;

                char *filename = &path[0];
                if(stat(filename, &st) == -1)
                {
                    s.writeClient("GET ERROR", connfd);
                    cout << "Error. File does not exist." << endl;
                    cout << "Handler Unlocked using Mutex" << endl;
                    pthread_mutex_unlock(&mylock);
                    drawLine();
                    continue;
                }

                off_t size = st.st_size; // total size in bytes
                input = input + "GET 0 " +  comm[2] + " ";
                input = input + to_string(size);
                //cout << input << endl;
                s.writeClient(input, connfd); // sent filesize to the client
                sleep(2);

                // Let us send the actual file to the server - In binary mode
                // 1. open the file
                FILE * fp;

                if (fp = fopen(filename, "r")) 
                    cout << "Sending File opened successfully" << endl;
                else
                    cout << "Error in sending file opening" << endl;

                char buffer[1024] = {0};
                int remain_data = size;

                // 2. send the file
                while(1)
                {
                    if(remain_data <= 0)
                    {
                        cout << "Sending of file completed (from server to client)." << endl;
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        fread(buffer, sizeof(char), remain_data, fp);
                        int bytes = send(connfd, buffer, remain_data, 0);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    fread(buffer, sizeof(char), 1024, fp);
                    int bytes = send(connfd, buffer, 1024, 0);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(fp);
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                continue;
            }
            else if(comm[1] == "1")
            {
                string input = "";
                struct stat st;

                // file path inside server
                string path = directory + comm[2];
                cout << "Source Directory of file: " << path << endl;

                char *filename = &path[0];
                if(stat(filename, &st) == -1)
                {
                    s.writeClient("GET ERROR", connfd);
                    cout << "Error. File does not exist." << endl;
                    cout << "Handler Unlocked using Mutex" << endl;
                    pthread_mutex_unlock(&mylock);
                    drawLine();
                    continue;
                }

                off_t size = st.st_size; // total size in bytes
                input = input + "GET 1 " +  comm[2] + " ";
                input = input + to_string(size);
                //cout << input << endl;
                s.writeClient(input, connfd); // sent filesize to the client
                sleep(2);

                // Let us send the actual file to the server - In binary mode
                // 1. open the file
                FILE * fp;

                if (fp = fopen(filename, "rb")) 
                    cout << "Sending File opened successfully" << endl;
                else
                    cout << "Error in sending file opening" << endl;

                char buffer[1024] = {0};
                int remain_data = size;

                // 2. send the file
                while(1)
                {
                    if(remain_data <= 0)
                    {
                        cout << "Sending of file completed (from server to client)." << endl;
                        break;
                    }
                    if(remain_data < 1024)
                    {
                        fread(buffer, sizeof(char), remain_data, fp);
                        int bytes = send(connfd, buffer, remain_data, 0);
                        remain_data -= bytes;
                        memset(buffer, 0, sizeof(buffer));
                        continue;
                    }
                    fread(buffer, sizeof(char), 1024, fp);
                    int bytes = send(connfd, buffer, 1024, 0);
                    remain_data -= bytes;
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(fp);
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                continue;
            }
        }
        else if(comm[0] == "SHOW")
        {
            showLogs("SHOW", connfd);
            if(client == "SHOW SELF")
            {
                s.writeClient("SHOW SELF", connfd);
                cout << "Handler Unlocked using Mutex" << endl;
                pthread_mutex_unlock(&mylock);
                drawLine();
                continue;
            }
            
            updateMetadata(directory);
            cout << "Handler Unlocked using Mutex" << endl;
            drawLine();
            s.writeClient("SHOW SERVER", connfd);
            pthread_mutex_unlock(&mylock);
            sleep(1);
            continue;
        }
        else if(comm[0] == "DELETE")
        {
            showLogs("DELETE", connfd);
            cout << "Request for DELETE command." << endl;
            s.writeClient(client, connfd);
            if(comm[1] == "SERVER")
            {
                string a = deleteFile(directory + comm[2]);
                sleep(1);
                s.writeClient(a, connfd);
            }
            cout << "Handler Unlocked using Mutex" << endl;
            pthread_mutex_unlock(&mylock);
            drawLine();
            continue;
        }
        else 
        {
            showLogs("INVALID COMMAND", connfd);
            s.writeClient("INVALID COMMAND", connfd);
        }

        cout << "Handler Unlocked using Mutex" << endl;
        pthread_mutex_unlock(&mylock);
    }

    s.closeServer(connfd);
	pthread_exit(NULL);
}

int main(int argc, char* argv[]) 
{
    // Making program for sending files from client to server
    if(argc < 2)
    {
        cout << "Error. Port Number is missing." << endl;
        exit(0);
    }

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