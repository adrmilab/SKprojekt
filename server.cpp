#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstring>

#define QUEUE_SIZE 6
#define BUFF 1024
#define NICK 16

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//class with client data given to thread
typedef struct thread_data
{
	int connection_socket_descriptor;
	char nick[NICK];
	struct thread_data *before;
	struct thread_data *after;
}thread_data_t;

//global client lists
thread_data_t *basic;
thread_data_t *modified;

//function that returns list of clients after adding new "CLEAR" one
thread_data_t *add_new(int connection_socket_fd)
{
	thread_data_t *client_list=(thread_data_t *)malloc(sizeof(thread_data_t));
	client_list->connection_socket_descriptor=connection_socket_fd;
	client_list->before=NULL;
	client_list->after=NULL;
	strcpy(client_list->nick,"EMPTY");
	return client_list;
}

//function that deals with connection new client
void* handleConnection(void *ready_client) {
    char name[NICK];
	char read_from[BUFF];
	int how_many_char_read=0;
	char write_to[BUFF];
	int how_many_char_write=0;
	thread_data_t *helper=(thread_data_t *)ready_client;
	bool disconnected=0;
	thread_data_t *helper_write;
	
	//setting nick
	how_many_char_read=read(helper->connection_socket_descriptor,name,NICK);
	if(how_many_char_read<0)
	{
		perror("error with reading nick");
		exit(1);
	}
	else if(how_many_char_read>NICK)
	{
		disconnected=1;
	}
	else
	{
		strcpy(helper->nick,name);
		cout<<"Client set nick: "<<helper->nick<<endl;
	}
	
	//main chat
	while(1)
	{	
		//checking if communication startd succesfully or is succesfully or if client is still connected
		if(disconnected==1)
		{
			break;
		}
		
		//reading chars
		how_many_char_read=read(helper->connection_socket_descriptor,read_from,BUFF);
		if(how_many_char_read>0)
		{
			if (strlen(read_from) == 0) 
			{
				cout<<read_from<<endl;
               continue;
            }	
		}
		else if(how_many_char_read==0 || strcmp(read_from, "exit") == 0)
		{
			disconnected=1;
		}	
		else if(how_many_char_read<0)
		{
			disconnected=1;
		}
		pthread_mutex_lock( &mutex);
		cout<<"Received message '"<<read_from<<" '"<<endl;
		//sending to all clients
		helper_write=basic->after;
		while(helper_write!=NULL)
		{
			if(helper->connection_socket_descriptor!=helper_write->connection_socket_descriptor)
			{	
				strcpy(write_to,read_from);
				//Creating a new temporary char array to concatenate the user nick
				int temp_buff_size = strlen(write_to) + strlen(helper->nick)+2;
				char* temp_concat = new char[temp_buff_size];
				strcpy(temp_concat, helper->nick);
				strcat(temp_concat,": ");
				strcat(temp_concat,write_to);
				
				how_many_char_write=write(helper_write->connection_socket_descriptor,temp_concat,BUFF);
				if(how_many_char_write<0)
				{
					perror("problem with writing");
				}
				delete[] temp_concat;
			}
			helper_write=helper_write->after;
		}
		pthread_mutex_unlock( &mutex );
	}
	
	//closing socket
	close(helper->connection_socket_descriptor);
	
	if(helper==modified)
	{
		modified=modified->before;
		modified->after=NULL;
	}
	else
	{
		modified->before->after=modified->after;
		modified->after->before=modified->before;
	}
	return 0;
}

//function that implements safe exit
void to_exit(int sig) {
    while (basic != NULL) {
		// close all socket
	
        close(basic->connection_socket_descriptor);  
        basic = basic->after;
    }
	
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	//catching ctrl-c and exit
	signal(SIGINT, to_exit);
	
	int server_socket_descriptor;
	struct sockaddr_in server_address, client_address;
	int bind_result;
	int listen_result;
	char reuse_addr_val = 1;
	int connection_socket_descriptor;
	int port;

	 if (argc != 2)
   {
     fprintf(stderr, "Sposób użycia: %s port_number\n", argv[0]);
     exit(1);
   }
	//creating server socket
	server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_descriptor < 0)
	{
		perror("error with creating server socket");
		exit(1);
	}
	setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
	
	//initialization
	port = atoi(argv[1]);
	memset(&server_address, 0, sizeof(server_address));
	memset(&client_address, 0, sizeof(client_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	//bind
	bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(server_address));
	if (bind_result < 0)
	{
		perror("error with binding");
		exit(1);
	}
	
	//listen
	listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
	if (listen_result < 0) {
		perror("error with listening");
		exit(1);
	}
	
	//global list initialization
	basic=add_new(server_socket_descriptor);
	modified=basic;
	
	int c_add_size=sizeof(client_address);
	while(1)
	{	
		
		connection_socket_descriptor=accept(server_socket_descriptor, (struct sockaddr*) &client_address, (socklen_t*) &c_add_size);
		if (connection_socket_descriptor < 0)
		{
			perror("error with accepting");
			exit(1);
		}
		pthread_mutex_lock( &mutex);

		cout<<"Client connected"<<endl;
		
		//appending list of connected clients
		thread_data_t *actual=add_new(connection_socket_descriptor);
		actual->before=modified;
		modified->after=actual;
		modified=actual;
		
		pthread_t handle;
        if (pthread_create(&handle, NULL, handleConnection, (void *)actual) != 0) {
            perror("creating pthread error");
            exit(1);
        }
		pthread_mutex_unlock( &mutex );

	}
   
   return(0);
}

