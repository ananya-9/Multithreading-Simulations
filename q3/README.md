# MULTI THREADED SERVER 

### Compilation Instructions 

- To compile the  code , run `g++ -o client client.cpp` in the terminal, to create a client executable.
- Run the command `g++ -o server server.cpp -lpthread` in the terminal to create a server executable.

_____

### Run Instructions 
- In separate terminal sessions , run the above generated executables in the order :
  1. `./server` to create a local server for the code. Enter the number of nodes, number of connections followed by list of connection.
  2. `./client` to create a client interface.
  3. Commands that can be entered on client terminal: 
  ```
        - pt
        - send <node_number> <message>
        - exit
    ```
_____
## Implementation:

### 1. Client Side 

- The program established a socket for the client, generated a sock_fd for the client which established connection for the client with the server hosted on port 8988.
- The client displays a prompt to take commands, tokenises them and sends a message to the server. On receiving command exit, it closes the connection to the server.

### 2. Server Side 
The nodes and their routing table details are stored in a global array of type struct node:
```c
struct node {
    pthread_t tid;
    int node_no;//0indexed
    // routing table for each node
	int *dest; // connections
    int *forw;
	int *delay; // delay
    int no_of_conns;
} **nodes;
```
Data to be transferred between nodes are stored using a structure packet:
```c
struct packet{
    int curr_node;
    int source;
    int dest;
    int forw;
    int port;
    string message;
} packet;
```
#### <b>Input</b>

The program stores all nodes as required and constructs basic routing tables- the direct path to every connection of each node.

#### <b>Optimizing routing tables</b>

-The program pre-optimizes the routing tables of *all* nodes. The function ```optimize_routing_tables ``` optimizes the routing table of the node sent as an argument. It uses the ```find_min_path``` function to find the path of minimum delay, and corresponding forward for every connection of the node. 

-```find_min_path``` function returns a pair containing the min delay and forw to reach given destination from given start node. It calls the recursive function ```create_min_path``` to get the minimum delay of each connection, by finding the path of each subsequent connection until the destination is reached.

```c
int create_min_path(int start_node, int dest_node)//returns minimum delay to reach dest_node from start node
{
    int delay,min_delay = 10000;

    if(start_node == dest_node)
    {
        return 0;
    }
    if(called[start_node] == 1)
    {
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
    return min_delay;
}
```
#### <b>Nodes </b>
The server is setup and hosted on the port 8988 using the system calls - bind, socket, listen, and it accepts the incoming client requests using the accept system call in the main server (representing node 0). 

- ```pt ```: If pt is received from client, server sends the optimized routing table of node 0.
- ```send <node_number> <message> ```:
    -  The server receives the message from the client.
    - It creates a packet to store relevant information and the message.
    - It checks the routing table and creates threads for 2 nodes- the source node, calling function ```send_from_node``` and the forwarded node, calling function ```receive_at_node```:
    ```c
    pthread_create(&(nodes[0]->tid), NULL, send_from_node, (void *)(&packet));
    pthread_create(&(nodes[packet.forw]->tid), NULL, recieve_at_node, NULL);
    ```
    
    - ```send_from_node``` converts the packet information to a string, and sets up a server on ```PORT``` for the sending node to listen for the client (receiving node). Once it accepts a connection, it sends the packet information as a string, receives an acknowledgement and closes the connection.

    - ```receive_at_node``` function establishes a socket for a client to denote the receiving node. It generates a sock_fd for the client which connects with the server on ```PORT```. ``` PORT``` is a global variable maintained by a mutex lock, and changed for each new socket connection.<br> The client receives a string from the sending node, and sends back an acknowledgement. It parses the string using ```parse_message``` function, which returns a packet.<br> It checks whether the message has reached the destination yet; if it hasn't, it constructs a new packet. It checks the routing table to find the forw to reach destination from the current node, and it creates threads for the current node (to send message), and forwarding node(to recieve the message). 
    ```c
    void *recieve_at_node(void *)
    {
        ...
        if(reached_dest == 0)
        {
            ...
            pthread_create(&(nodes[packet.curr_node]->tid), NULL, send_from_node, (void *)(&packet));
            
            sleep(nodes[packet.curr_node]->delay[j]);
            pthread_create(&(nodes[packet.forw]->tid), NULL, recieve_at_node, NULL);
            pthread_join(nodes[packet.forw]->tid, NULL);
            pthread_join(nodes[packet.curr_node]->tid, NULL);
        }

        return NULL;
    }
    ```

    - <b>Therefore, the message is forwarded through nodes (represented by threads) until it reaches the destination.</b>



