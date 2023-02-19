#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <assert.h>
#include <queue>
#include <vector>
#include <tuple>
using namespace std;

typedef long long LL;
const LL MOD = 1000000007;
#define part cout << "-----------------------------------" << endl;
#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
const LL buff_sz = 1048576;

pair<string, int> read_string_from_socket(int fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    // debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        // return "
        exit(-1);
    }

    // debug(output);
    output[bytes_received] = 0;
    output.resize(bytes_received);

    return {output, bytes_received};
}
int send_string_on_socket(int fd, const string &s)
{
    // cout << "We are sending " << s << endl;
    int bytes_sent = write(fd, s.c_str(), s.length());
    // debug(bytes_sent);
    // debug(s);
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA on socket.\n";
        // return "
        exit(-1);
    }

    return bytes_sent;
}

// Driver Code
int main()
{
	int network_socket;

	// Create a stream socket
	network_socket = socket(AF_INET,
							SOCK_STREAM, 0);
	if(network_socket < 0) 
	{
		fprintf(stderr,"\033[0;31m");
    	perror("Error in socket");
    	fprintf(stderr,"\033[0m");
    	exit(1);
    }
	// Initialise port number and address
	struct sockaddr_in server_address;

	memset(&server_address, '0', sizeof(server_address));  // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below
    
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(8988);

	// Converts an IP address in numbers-and-dots notation into either a 
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0)
    {
        // printf("\nInvalid address/ Address not supported ");
        cout<<"\nInvalid address/ Address not supported \n";
        return -1;
    }
	// Initiate a socket connection to server
	int connection_status = connect(network_socket, (struct sockaddr*)&server_address,
									sizeof(server_address));

	// Check for connection error
	if (connection_status < 0) {
		perror("Error");
		return 0;
	}

	// printf("Connection established\n");
    cout<<"Connection established\n";
	int numCom = 0;
	char tokens[200][1024];
	//shell
	while(1)
	{
    	char buffer[1024] = {0};
    	char sz[100] = {0};
  		//printf("\033[1;36mclient> \033[0m");
        cout<<"\033[1;36mclient> \033[0m";
  		size_t inpSize = 0;
        char *input; char inp[1000];
        if(getline(&input,&inpSize,stdin)==-1)
		{
			perror("input");
			break;
        }
        int l = strlen(input);
        
        
        char input_copy[l];
        for(int j=0;j<l-1;j++)
            input_copy[j]=input[j];
        input_copy[l-1] = '\0';
        // strcpy(input_copy, input);

        char* token = strtok(input, " \n\t");    
		numCom = 0;  
        //split input based on spaces, newlines and tabs
        while (token != NULL) 
		{ 
    	//  command(token); 
    	//  printf("%s\n",token );
          	strcpy(tokens[numCom],token);
          	++numCom;
          	token = strtok(NULL, " \n\t"); 
        } 
		if(numCom==1)
		{
			if(!strcmp(tokens[0],"pt"))
			{
                send_string_on_socket(network_socket, tokens[0]);
                int num_bytes_read;
                string output_msg;
                tie(output_msg, num_bytes_read) = read_string_from_socket(network_socket, buff_sz);
                cout << output_msg;
                cout << "====" << endl;
			}
			else if(!strcmp(tokens[0],"send"))
			{
				//printf("Please provide destination node and message");
                cout<<"Please provide destination node and message"<<endl;
			}
            else if(!strcmp(tokens[0],"exit"))
			{
                send_string_on_socket(network_socket, tokens[0]);
            	break;
          	}
			else
			{
                cout<<"Command not found\n";
				//printf("Command not found");
			}
        }
		else if(numCom>1)
		{
          	if(!strcmp(tokens[0],"send"))
		  	{
                
                send_string_on_socket(network_socket, input_copy);

                //break;
          	}
			else if(!strcmp(tokens[0],"exit"))
			{
                send_string_on_socket(network_socket, tokens[0]);
            	break;
          	}
			else
			{
				//printf("Command not found");
                cout<<"Command not found\n";
          	}
		}
    }

	//sending data to socket
	// Send data to the socket
	int client_request = 1;
	send(network_socket, &client_request, sizeof(client_request), 0);

	// Close the connection

  	//printf("Closing the connection.");
    cout<<"Closing the connection\n";
	close(network_socket);
	return 0;

}
