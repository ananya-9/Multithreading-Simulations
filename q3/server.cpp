/* IDEA : Set up the server side for all nodes. Means create the socket connection of server type, and keep listening for clients.
When a send comes from CLIENT to main server, the to send data:
    sender should accept a connection from req client
    reciever should set a client side and connect to server socket
    the recieve data
    
Notes; Sleep for delay amt of time- where
       are the setup socket identical for server and client?*/



#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <tuple>
#include <vector>

using namespace std;

typedef long long LL;

//sockaddr_storage
#define MAX_CLIENTS 4
#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;


const int initial_msg_len = 256;

////////////////////////////////////
int node_rt_sizes[1000] = {0}; //exists[i] is 0 if the nodes[i] construct doesn't exist yet. Otherwise it will be the number of connections that node has

//const LL buff_sz = 1048576;
const LL buff_sz = 1048576;
int n, m;//the number of nodes in the network n and the number of direct connections between them m
int ports[100];
int PORT = 8000;
pthread_mutex_t port_lock;
struct node {
    pthread_t tid;
    int node_no;//0indexed
    // routing table for each node
	int *dest; // connections
    int *forw;
	int *delay; // delay
    int no_of_conns;
} **nodes;

struct packet{
    int curr_node;
    int source;
    int dest;
    int forw;
    int port;
    string message;
} packet;
//pthread_t tid;

pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);send 4 hi

    return {output, bytes_received};
}
int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}


void *send_from_node(void *arg)
{
    struct packet *packet_ptr = (struct packet *)arg;
    //Creating sockets for connection
    int i;
    int client_socket_fd, port_number;
	int serverSocket;// newSocket;
    struct sockaddr_in serverAddr, client_addr_obj;
    //struct sockaddr_storage serverStorage;
 
    //socklen_t addr_size;
    socklen_t clilen;
 
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("sending node: ERROR creating welcoming socket");
        exit(-1);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    
    serverAddr.sin_port = htons(PORT);
    cout<<"Port of sending node = "<<serverAddr.sin_port<<endl;
 
    // Bind the socket to the
    // address and port number.
    if(bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0)
    {
        perror("sending node: Error on bind on welcome socket: ");
        exit(-1);
    }
//create a socket on a port- from ports array
//try to connect to a client on that port
if (listen(serverSocket, MAX_CLIENTS) == 0)
        printf("sending node: Listening at server node\n");
    else
        printf("sending node: Error\n");
 
    i = 0;
    string cmd;
    int received_num, ret_val=1;
    
    //addr_size = sizeof(serverStorage);
    clilen = sizeof(client_addr_obj);
    // Extract the first
    // connection in the queue
    client_socket_fd = accept(serverSocket, (struct sockaddr *)&client_addr_obj, &clilen);
    if (client_socket_fd < 0)
    {
        perror("sending node: ERROR while accept() system call occurred in SERVER node");
        exit(-1);
    }
    printf("sending node: New client node connected from port number %d and IP %s \n", ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
    string msg_to_send_back = "<C>"+to_string(packet_ptr->curr_node)+"<S>"+to_string(packet_ptr->source)+"<D>"+to_string(packet_ptr->dest)+"<F>"+to_string(packet_ptr->forw)+"<M>"+packet_ptr->message+"\n";
    int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
            // debug(sent_to_client);
        cout<<"sending node: msg sent is "<< msg_to_send_back<<"\n";

    if (sent_to_client == -1)
    {
        perror("sending node: Error while writing to client. Seems socket has been closed");
        goto close_client_socket_ceremony;
    }

    
    tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
    ret_val = received_num;
    // debug(ret_val);
    // printf("Read something\n");
    if (ret_val <= 0)
    {
        // perror("Error read()");
        printf("Server could not read command from client\n");
        goto close_client_socket_ceremony;
    }
    cout << "Client sent : " << cmd << endl;
close_client_socket_ceremony:
    close(client_socket_fd);
    printf("sending node: Server node has disconnected from client node\n");
    

    close(serverSocket);
    return NULL;

}

struct packet parse_message(string msg)
{
    int l = msg.length();
    struct packet p;
    string curr, sour, dest, forw, message;
    int i=3;
    int j=0;
    while(msg[i]!='<')
    {
        curr[j]=msg[i];
        j++;
        i++;
    }
    i=i+3;
    j=0;
    while(msg[i]!='<')
    {
        sour[j]=msg[i];
        j++;
        i++;
    }
    i=i+3;
    j=0;
    while(msg[i]!='<')
    {
        dest[j]=msg[i];
        j++;
        i++;
    }
    i=i+3;
    j=0;
    while(msg[i]!='<')
    {
        forw[j]=msg[i];
        j++;
        i++;
    }
    i=i+3;
    // j=0;
    // while(msg[i]!='\n')
    // {
    //     p.message[j]=msg[i];
        
    //     j++;
    //     i++;
    // }
    p.message = msg.substr(i, l-i-1);
    
    p.curr_node = stoi(curr);
    p.source = stoi(sour);
    p.dest = stoi(dest);
    p.forw = stoi(forw);
    //p.message = message;
    //cout<<"parse fn: Struct msg element "<<p.message<<endl;
    return p;
}

void *recieve_at_node(void *)
{
    
    //int *port= (int *)arg;
//create a socket on same port and connect with the server

    int network_socket;

	// Create a stream socket
	network_socket = socket(AF_INET,
							SOCK_STREAM, 0);
	if(network_socket < 0) 
	{
		fprintf(stderr,"\033[0;31m");
    	perror("receiving node: Error in socket");
    	fprintf(stderr,"\033[0m");
    	exit(1);
    }
	// Initialise port number and address
	struct sockaddr_in server_address;

	memset(&server_address, '0', sizeof(server_address));  // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below
    
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);
    cout<<"Port of receiving node is "<<server_address.sin_port<<endl;
    if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0)
    {
        // printf("\nInvalid address/ Address not supported ");
        cout<<"\nreceiving node: Invalid address/ Address not supported \n";
        return 0;
    }
	// Initiate a socket connection to server
	int connection_status = connect(network_socket, (struct sockaddr*)&server_address,
									sizeof(server_address));

	// Check for connection error
	if (connection_status < 0) {
		perror("receiving node: Error");
		return 0;
	}
    cout<<"receiving node: Connection established from client node\n";
    int num_bytes_read;
    string packet_message;
    int reached_dest = 1;
    tie(packet_message, num_bytes_read) = read_string_from_socket(network_socket, buff_sz);
    struct packet rec_packet= parse_message(packet_message);
    //create new packet from rec_packet
    struct packet packet;
    packet.curr_node = rec_packet.forw;
    packet.dest = rec_packet.dest;
    packet.forw = packet.curr_node;//find from table
    int j = 0;
    if(packet.curr_node != packet.dest)
    {
        reached_dest = 0;
        for(j=0;j<nodes[packet.curr_node]->no_of_conns; j++)
        {
            if(nodes[packet.curr_node]->dest[j] == packet.dest)   
            {
                packet.forw = nodes[packet.curr_node]->forw[j];
                break;
            }
        }
    }
    packet.source = rec_packet.source;
    packet.message = rec_packet.message;
    
    cout<<"\033[1;36mData received at node: "<<packet.curr_node<<" : Source : "<<packet.source<<"; Destination : "<<packet.dest<<"; Fowarded_Destination : "<<packet.forw<<"; Message : "<<packet.message<<";\n\033[0m";
    
    //cout<<"receiving node: Message is "<<rec_packet.message<<endl;

    string ack_msg = "Receieved data at node";
    send_string_on_socket(network_socket, ack_msg);

    cout<<"receiving node: Closing the connection at client node\n";
	close(network_socket);

    //If msg hasn't reached, forward msg as required
    if(reached_dest == 0)
    {
        // packet.port = rec_packet.port++;
        // (*port) += 1;
        pthread_mutex_lock(&port_lock);
        PORT++;
        pthread_mutex_unlock(&port_lock);
        pthread_create(&(nodes[packet.curr_node]->tid), NULL, send_from_node, (void *)(&packet));
            //pthread_create(&(nodes[packet->forw]->tid), NULL, recieve_at_node, (void *)nodes[packet->forw]->node_no);
        sleep(nodes[packet.curr_node]->delay[j]);
        pthread_create(&(nodes[packet.forw]->tid), NULL, recieve_at_node, NULL);
        pthread_join(nodes[packet.forw]->tid, NULL);//when
        pthread_join(nodes[packet.curr_node]->tid, NULL);//when
    }
    return NULL;
//get data cout<<"Data received at node: node_num : Source : 0; Destination : 5; Fowarded_Destination : 3; Message :  ";
//if destination of recieved packet == node_num print recieved and return;
//else pthread_create(nodes[forw for fastest path to dest from node_num])

}


void handle_connection(int client_socket_fd)
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;
    int read_size = (sizeof(char)*4)/8;
    /* read message from client */
    int ret_val = 1;
    while (1)
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        // debug(ret_val);
        // printf("Read something\n");
        if (ret_val <= 0)
        {
            // perror("Error read()");
            printf("Server could not read command from client\n");
            goto close_client_socket_ceremony;
        }
        //cout << "Client sent : " << cmd << endl;
        if (cmd == "pt")
        {
            string rt = "";
            string temp = "";
            for(int j=0;j<node_rt_sizes[0];j++)
            {
                temp = to_string(nodes[0]->dest[j]) + "\t" + to_string(nodes[0]->forw[j]) + "\t" + to_string(nodes[0]->delay[j]) + "\n";
                rt = rt + temp;
            }
            string msg_to_send_back = "dest\tforw\tdelay\n" + rt;
            ////////////////////////////////////////
            // "If the server write a message on the socket and then close it before the client's read. Will the client be able to read the message?"
            // Yes. The client will get the data that was sent before the FIN packet that closes the socket.

            int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
            // debug(sent_to_client);
            if (sent_to_client == -1)
            {
                perror("Error while writing to client. Seems socket has been closed");
                goto close_client_socket_ceremony;
            }
            cout<<"____________________________________________\n\n";
        }
        if (cmd[0] == 's')
        {
            string dest_node, message = ""; 
            dest_node = cmd[5];
            for(int i=7;i<cmd.length();i++)
            {
                
                message= message + cmd[i];
            }
            //Need to send message to destination node
            int num = stoi(dest_node);
            //packet = (struct packet *)malloc(sizeof(struct packet));
            packet.curr_node = 0;
            packet.dest = num;
            packet.forw = 0;//find from table
            int j;
            for(j=0;j<nodes[0]->no_of_conns; j++)
            {
                if(nodes[0]->dest[j] == num)   
                {
                    packet.forw = nodes[0]->forw[j];
                    break;
                }
            }
            packet.source = 0;
            packet.message = message;
            packet.port = 8000 + (rand()%1000);
            // int q = packet.port;
            // int *rec_port;
            // rec_port = &q;
            pthread_mutex_lock(&port_lock);
            PORT++;
            pthread_mutex_unlock(&port_lock);
           
            cout<<"\033[1;36mData received at node: 0 : Source : 0; Destination : "<<num<<"; Fowarded_Destination : "<<packet.forw<<"; Message : "<<message<<";\n\033[0m";
            pthread_create(&(nodes[0]->tid), NULL, send_from_node, (void *)(&packet));
            //pthread_create(&(nodes[packet->forw]->tid), NULL, recieve_at_node, (void *)nodes[packet->forw]->node_no);
            sleep(nodes[0]->delay[j]);
            pthread_create(&(nodes[packet.forw]->tid), NULL, recieve_at_node, NULL);
            pthread_join(nodes[packet.forw]->tid, NULL);//when
            pthread_join(nodes[0]->tid, NULL);//when
            cout<<"____________________________________________\n\n";

        }
        if (cmd == "exit")
        {
            cout << "Exit pressed by client" << endl;
            goto close_client_socket_ceremony;
        }
    }
close_client_socket_ceremony:
    cout<<"____________________________________________\n\n";

    close(client_socket_fd);
    printf("Disconnected from client\n");
    // return NULL;
}

/// FUNCTIONS FOR OPTIMIZING ROUTING TABLE /////////////////////////////////////////////////////////////////////////////////////////////

int called[100]; //to keep trackof which create_min_path fns were already called, to cut off the recursion if it starts cycling. 
                //i.e. If (0,3) calls (2,3) calls (1,3) calls (0,3), that line will continue infinitely without reaching 3 so cut it off and assign it max 
                //to make sure it won't get added to minimum
int create_min_path(int start_node, int dest_node)//returns minimum delay to reach dest_node from start node
{
    //cout<<"create_min_path("<<start_node<<", "<<dest_node<<") entered. ";
    int delay,min_delay = 10000;

    if(start_node == dest_node)
    {
        // cout<<"returning 0\n";
        return 0;
    }
    if(called[start_node] == 1)
    {
        // cout<<"returning 10000\n";
        return 10000;
    }
    called[start_node] = 1;
    int temp[100];
    for(int i = 0;i<20;i++)
        temp[i]=called[i];
    for(int i = 0; i<nodes[start_node]->no_of_conns; i++)
    {
        delay = create_min_path(nodes[start_node]->dest[i], dest_node) + nodes[start_node]->delay[i];
        if(delay < min_delay)
        {
            min_delay = delay;
        }
    }
    //restore called everytime it finishes calling all nodes of current node
    //so that for a new function call, it doesn't cutoff unecessarily
    for(int i = 0;i<20;i++)
        called[i]=temp[i];
    // cout<<"returning "<<min_delay<<endl;
    return min_delay;
}

pair<int, int> find_min_path(int start_node_no, int dest_node_no) //Returns where msg should be forwarded from 
{                                                                 //start_node_no to reach dest_node_no with least delay 
                                                                  //and the delay value
    pair<int, int>forw_del;
    //cout<<"FInd min path of "<<start_node_no<<" "<<dest_node_no<<endl;
    for(int i = 0;i<20;i++)
        called[i]=0;
    
    called[start_node_no] = 1;
    int delay,min_delay = 10000;
    int forw = dest_node_no;
    //Now simulate all paths, and compare delays, and replace forw if delay is lower
    for(int i = 0; i<nodes[start_node_no]->no_of_conns; i++)
    {
        if(nodes[start_node_no]->dest[i]== dest_node_no)
            delay = nodes[start_node_no]->delay[i];
        else
        {
            for(int i = 0;i<20;i++)
                called[i]=0;
    
            called[start_node_no] = 1;
            delay = create_min_path(nodes[start_node_no]->dest[i], dest_node_no) + nodes[start_node_no]->delay[i];
            
        }
        if(delay < min_delay)
        {
            min_delay = delay;
            forw = nodes[start_node_no]->dest[i];
        }
    }
    forw_del.first = forw;
    forw_del.second = min_delay;
    return forw_del;
}
void optimize_routing_tables(struct node* node)
{
    pair<int, int> forw_del;    
    for(int j=0;j<node->no_of_conns;j++)//for every connection as destination
    //for(int j=0;j<n;j++)//for every connection as destination
        //find all possible options of reaching all end nodes of node and pick it's corresponding forward
    {
        forw_del = find_min_path(node->node_no, node->dest[j]);
        node->forw[j] = forw_del.first;
        node->delay[j] = forw_del.second;
    }
    
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Driver Code
int main()
{
    //input - number of nodes, connections between them; 
	scanf("%d %d", &n, &m);
    int a[m],b[m],d[m];
    nodes = (struct node **)malloc(sizeof(struct node *) * (n));
    for(int j = 0; j<m; j++)
    {
        scanf("%d %d %d", &a[j], &b[j], &d[j]);
        if(node_rt_sizes[a[j]] == 0)
        {
            nodes[a[j]] = (struct node *)malloc(sizeof(struct node));
        }
        node_rt_sizes[a[j]]++;
        if(node_rt_sizes[b[j]] == 0)
        {
            nodes[b[j]] = (struct node *)malloc(sizeof(struct node));
        }
        node_rt_sizes[b[j]]++;

        //assign ports
        ports[j]=8000+j;
    }
    //assign required mem to routing tables
    nodes[0]->dest = (int *)malloc(sizeof(int)*(n-1));
    nodes[0]->forw = (int *)malloc(sizeof(int)*(n-1));
    nodes[0]->delay = (int *)malloc(sizeof(int)*(n-1));
    nodes[0]->no_of_conns = 0;
    nodes[0]->node_no = 0;    for(int j=1;j<n;j++)
    {
        nodes[j]->dest = (int *)malloc(sizeof(int)*(node_rt_sizes[j]));
        nodes[j]->forw = (int *)malloc(sizeof(int)*(node_rt_sizes[j]));
        nodes[j]->delay = (int *)malloc(sizeof(int)*(node_rt_sizes[j]));
        
        nodes[j]->no_of_conns = 0;
        nodes[j]->node_no = j;
    }
    //build the unoptimised routing tables
    for(int j=0;j<m;j++)
    {
        // nodes[a[j]]->dest[nodes[a[j]]->no_of_conns] = b[j];
        // nodes[a[j]]->forw[nodes[a[j]]->no_of_conns] = b[j];
        // nodes[a[j]]->delay[nodes[a[j]]->no_of_conns] = d[j];
        nodes[a[j]]->dest[nodes[a[j]]->no_of_conns] = b[j];
        nodes[a[j]]->forw[nodes[a[j]]->no_of_conns] = b[j];
        nodes[a[j]]->delay[nodes[a[j]]->no_of_conns] = d[j];
        nodes[a[j]]->no_of_conns++;

        nodes[b[j]]->dest[nodes[b[j]]->no_of_conns] = a[j];
        nodes[b[j]]->forw[nodes[b[j]]->no_of_conns] = a[j];
        nodes[b[j]]->delay[nodes[b[j]]->no_of_conns] = d[j];
        nodes[b[j]]->no_of_conns++;
    }
    for(int j=nodes[0]->no_of_conns;j<n-1;j++)
    {
        nodes[0]->dest[j] = j+1;
        nodes[0]->forw[j] = j+1;
        nodes[0]->delay[j] = 10000;

    }
    for(int j=nodes[1]->no_of_conns;j<n-1;j++)
    {
        nodes[1]->dest[j] = j;
        nodes[1]->forw[j] = j;
        nodes[1]->delay[j] = 10000;

    }
    nodes[0]->no_of_conns = n-1;
    node_rt_sizes[0] = n-1;
    nodes[1]->no_of_conns = n-1;
    node_rt_sizes[1] = n-1;
    for(int j=0;j<n;j++)
        optimize_routing_tables(nodes[j]);//optimise the routing tables
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Creating thread for each node



    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Creating sockets for connection
    int i;
    int client_socket_fd, port_number;
	int serverSocket;// newSocket;
    struct sockaddr_in serverAddr, client_addr_obj;
    //struct sockaddr_storage serverStorage;
 
    //socklen_t addr_size;
    socklen_t clilen;
 
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }
    bzero((char *)&serverAddr, sizeof(serverAddr));

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8988);
 
    // Bind the socket to the
    // address and port number.
    if(bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
 
    // Listen on the socket,
    // with 40 max connection
    // requests queued
    if (listen(serverSocket, MAX_CLIENTS) == 0)
        printf("Listening\n");
    else
        printf("Error\n");
 
    i = 0;
 
    while (1) 
    {
        //addr_size = sizeof(serverStorage);
        clilen = sizeof(client_addr_obj);
        // Extract the first
        // connection in the queue
        client_socket_fd = accept(serverSocket, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }
        printf("New client connected from port number %d and IP %s \n", ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        handle_connection(client_socket_fd);

    }
    close(serverSocket);
    return 0;
}
