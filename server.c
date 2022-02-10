#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>

#define ROOM_SIZE 3
#define STDIN 0
#define STDOUT 1
#define STDERR 2

int room_members[4][10][ROOM_SIZE];
int room_size[4][10] = {0};
int client_fd[120]; 
char client_fd_type[120];
int client_count = 0; 
char buffer[255];
char print_str[256];

int accept_new_connection(int server_socket)
{
    socklen_t client_size;
    struct sockaddr_in client_address;
    client_size = sizeof(client_address);
    int new_socket_fd = accept(server_socket, (struct sockaddr *) &client_address, &client_size);
    if (new_socket_fd < 0)
    {
        write(STDERR, "ERROR: accept new connection failed\n", strlen("ERROR: accept new connection failed\n"));
        exit(1);
    } 
    return new_socket_fd;
}

void handle_room(int room_type, int room_num)
{
	for (int i = 0 ; i < ROOM_SIZE ; i++)
	{
		int cur_fd = room_members[room_type][room_num][i];
		bzero(buffer, 255);
		strcat(buffer, "Questions and Answers began:");
		write(cur_fd, buffer, strlen(buffer));
	}
}

int assign_room_to_client(int clientfd, char room_type)
{
    int room_num = 0;
    for(int i = 0; i < 10; i++)
        if(room_size[room_type - '0'][i] < ROOM_SIZE)
        {
            room_num = i;
            break;
        }     
	room_members[room_type - '0'][room_num][room_size[room_type - '0'][room_num]] = clientfd;
    client_fd_type[clientfd] = room_type;
    room_size[room_type - '0'][room_num]++;
    char turn[2] = {room_size[room_type - '0'][room_num] + '0', '\0'};
    bzero(buffer, 255);
    strcat(buffer, "Please connect to port ");
    char port_num[5] = {'8', '0', room_type, room_num + '0', '\0'};
    strcat(buffer, port_num);
    strcat(buffer, ". You are person number ");
    strcat(buffer, turn);
    strcat(buffer, " in this room for Questions and Answers.\n\0");
    if (write(clientfd, buffer, strlen(buffer)) < 0)
    {
        write(STDERR, "ERROR: wiring message from server to the client failed\n", strlen("ERROR: wiring message from server to the client failed\n"));
        exit(1);
    }
	if (room_size[room_type - '0'][room_num] == ROOM_SIZE) 
		handle_room(room_type - '0', room_num);
 }

void write_to_file(int clientfd)
{
    char file_address[256];
    bzero(file_address, 255);
    strcat(file_address, "Q&A");
    file_address[3] = client_fd_type[clientfd];
    strcat(file_address, ".txt");
    int file_fd;
    file_fd = open(file_address, O_APPEND | O_RDWR);
    if(file_fd < 0)
        file_fd = open(file_address, O_CREAT | O_APPEND | O_RDWR);
    write(file_fd, buffer, strlen(buffer));
    close(file_fd);
}

int handle_connection(int clientfd)
{
    bzero(buffer,255);
    int error_check = read(clientfd, buffer, 255);
    if (error_check <= 0)
    {
        if (error_check == 0) 
            return 0;
        write(STDERR, "ERROR: reading from client failed\n", strlen("ERROR: reading from client failed\n"));
        exit(1); 
    }
    bzero(print_str, 255);
    strcat(print_str, "MESSAGE FROM CLIENT WITH FD = ");
    char num[5];
    sprintf(num, "%d" , clientfd);
    strcat(print_str, num);
    strcat(print_str, ": ");
    strcat(print_str, buffer);
    strcat(print_str, "\n\n");
    write(STDIN, print_str, strlen(print_str));
    if (buffer[0] == 'R')
        assign_room_to_client(clientfd, buffer[2]);
    if(buffer[1] == 'A')
        write_to_file(clientfd);
    return 1;
}

int main(int argc, char const *argv[])
{
    int port, sockfd;
    if (argc < 2) 
    {
        write(STDERR, "ERROR: port not found\n", strlen("ERROR: port not found\n"));
        exit(1);
    }
    port = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0)
    {
        write(STDERR, "ERROR: can't opening server socket\n", strlen("ERROR: can't opening server socket\n")); 
        exit(1); 
    }
    struct sockaddr_in server_address; 
    bzero((char *) &server_address, sizeof(server_address)); 
    server_address.sin_port = htons(port);  
    server_address.sin_addr.s_addr = INADDR_ANY; 
    server_address.sin_family = AF_INET;
    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        write(STDERR, "ERROR: binding failed\n", strlen("ERROR: binding failed\n")); 
        exit(1); 
    }
    listen(sockfd,60);
    fd_set current_socket, ready_socket;
    FD_ZERO(&current_socket);
    FD_SET(sockfd, &current_socket);
    write(1, "Server is Running...\n\n", 22);
    while (1)
    {
        ready_socket = current_socket;
    	if (select(FD_SETSIZE, &ready_socket, NULL, NULL, NULL) < 0)
        {
            write(STDERR, "ERROR: select failed\n", strlen("ERROR: select failed\n")); 
            exit(1);
        }
        for (int i = 0 ; i < FD_SETSIZE ; i++)
        {
            if (FD_ISSET(i, &ready_socket)) 
            {
                if (i == sockfd)
                {
                    bzero(print_str, 255);
                    write(STDIN, "New connection request from a client is found...\n", strlen("New connection request from a client is found...\n"));
                    int client_socket = accept_new_connection(sockfd);
                    client_fd[client_count++] = client_socket;
                    FD_SET(client_socket, &current_socket);
                    bzero(print_str, 255);
                    strcat(print_str, "New connection request accepted with fd = ");
                    char num[5];
                    sprintf(num, "%d" , client_socket);
                    strcat(print_str, num);
                    strcat(print_str, "\n");
                    write(STDIN, print_str, strlen(print_str));
                    bzero(print_str, 255);
                    strcat(print_str, "Number of Clients: ");
                    sprintf(num, "%d" , client_count);
                    strcat(print_str, num);
                    strcat(print_str, "\n\n");
                    write(STDIN, print_str, strlen(print_str));
                    bzero(buffer, 255);
                    strcat(buffer, "Room id: ComputerEngineering = 0  ElectricalEngineering = 1  CivilEngineering = 2  MechanicalEngineering = 3\n");
                    strcat(buffer, "\nTo enter the room you want, enter the room id in this format:  R <room_id>\n");
                    int n = write(client_socket, buffer, strlen(buffer));
                    if (n < 0)
                    {
                        write(STDERR, "ERROR: writing to client socket failed\n", strlen("ERROR: writing to client socket failed\n"));
                        exit(1); 
                    } 
                }
                else
                {
                    int n = handle_connection(i);
                    if (n == 0) 
                        FD_CLR(i, &current_socket);
                }
            }
        }
    }
    return 0;
}