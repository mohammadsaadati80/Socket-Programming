#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#include <time.h>
#include <errno.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define TIMER 60
#define ROOM_CAPACITY 3

char room_type = '0';
char best_answer[256];
char all_messages[256];
char print_str[256];
char buffer[256];
int udp_port, udp_socket_fd;
int port, socket_fd;
struct hostent *server;
struct sockaddr_in server_address;
fd_set cur_socket;
volatile sig_atomic_t time_is_over = 0;
int turn_counter = 0;
int my_turn = -1;

void signal_handler(int signum)
{ 
    time_is_over = 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        write(STDERR, "ERROR: hostname port not found\n", strlen("ERROR hostname port not found\n")); 
        exit(0);
    }
    port = atoi(argv[1]);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        write(STDERR, "ERROR: failed to open socket\n", strlen("ERROR: failed to open socket\n")); 
        exit(0);
    }       
    server = gethostbyname("127.0.0.1");
    if (server == NULL) 
    {
        write(STDERR, "ERROR: host not found\n", strlen("ERROR: host not found\n")); 
        exit(0);
    }
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_family = AF_INET;
    if (connect(socket_fd,(struct sockaddr *) &server_address,sizeof(server_address)) < 0) 
    {
        write(STDERR, "ERROR: connecting failed\n", strlen("ERROR: connecting failed\n")); 
        exit(0);
    }
    write(1, "Client successfully connected to the server\n", strlen("Client successfully connected to the server\n"));
    while (1)
    {
        FD_ZERO(&cur_socket);
        FD_SET(socket_fd, &cur_socket);
        FD_SET(STDIN, &cur_socket); 
        if (select(socket_fd + 1, &cur_socket, NULL, NULL, NULL) < 0)
            write(STDERR, "ERROR: select failed\n", strlen("ERROR: select failed\n"));
        if (FD_ISSET(socket_fd, &cur_socket)) 
        {
            if (socket_fd == socket_fd) 
            {
                bzero(buffer, 255);
                if (read(socket_fd, buffer, 255) < 0)
                {
                    write(STDERR, "ERR: reading buffer from server  failedfor this client failed\n", strlen("ERR: reading buffer from server  failedfor this client failed\n"));
                    exit(0);
                }
                bzero(print_str, 255);
                strcat(print_str, "\nMESSAGE FROM SERVER: ");
                strcat(print_str, buffer);
                strcat(print_str, "\n\n");
                write(STDIN, print_str, strlen(print_str));
                if (buffer[0] == 'Q') 
                {
                    FD_CLR(socket_fd, &cur_socket);
                    FD_CLR(STDIN, &cur_socket);
                    break;
                }
                if (buffer[0] == 'P') 
                {
                    write(STDOUT, "Connecting to UDP Socket...\n\n", strlen("Connecting to UDP Socket...\n\n"));
                    char port_number[4] = {'8', '0', buffer[25], buffer[26]};
                    room_type = buffer[25];
                    udp_port = atoi(port_number);
                }
            }
        }
        if (FD_ISSET(STDIN, &cur_socket))
        {
            bzero(buffer, 255);
            fgets(buffer, 255, stdin);
            if (write(socket_fd, buffer, strlen(buffer)) < 0)
            {
                write(STDERR, "ERROR: room is invalid\n", strlen("ERROR: room is invalid\n"));
                exit(0);
            }     
        }
    }
    int re_use_address_en = 1, broadcast_en = 1, opt_en = 1;
    struct sockaddr_in bc_address_send_to;
    struct sockaddr_in bc_address_recvie_from;
    udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd < 0)
    {
        write(STDERR, "ERROR: failed to open socket\n", strlen("ERROR: failed to open socket\n"));
        exit(0);
    }
    bc_address_send_to.sin_port = htons(udp_port);
    bc_address_send_to.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    bc_address_send_to.sin_family = AF_INET;
    setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &re_use_address_en, sizeof(re_use_address_en));
    setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt_en, sizeof(opt_en));
    setsockopt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_en, sizeof(broadcast_en)); 
    bc_address_recvie_from.sin_port = htons(udp_port); 
    bc_address_recvie_from.sin_addr.s_addr = htonl(INADDR_ANY); 
    bc_address_recvie_from.sin_family = AF_INET; 
    if (bind(udp_socket_fd, (struct sockaddr *) &bc_address_recvie_from, sizeof(bc_address_recvie_from)) < 0)
    {
        write(STDERR, "ERROR: binding failed\n", strlen("ERROR: binding failed\n"));
        exit(0);
    }
    write(STDOUT, "This client successfully connected to the UDP Socket \n\n", strlen("This client successfully connected to the UDP Socket \n\n"));
    signal(SIGALRM, signal_handler); 
    write(STDOUT, "Ask or Answer the Question:\n", strlen("Ask or Answer the Question:\n"));
    alarm(TIMER);
    while (1)
    {
        signal(SIGALRM, signal_handler); 
        FD_ZERO(&cur_socket);
        FD_SET(udp_socket_fd, &cur_socket);
        FD_SET(STDIN, &cur_socket); 
        if (select(udp_socket_fd + 1, &cur_socket, NULL, NULL, NULL) == -1)
        {
            if (errno != EINTR)
            {
                write(STDERR, "ERROR: select failed\n", strlen("ERROR: select failed\n")); 
                exit(0);
            }
            else
            {
                turn_counter++;
                time_is_over = 0;
                write(STDOUT, "Your turn is over\n",strlen("Your turn is over\n"));
                alarm(TIMER);
                continue;
            }               
        }   
        if (FD_ISSET(STDIN, &cur_socket)) 
        {
            bzero(buffer, 255);
            fgets(buffer, 255, stdin);
            char message[256];
            bzero(message, 255);
            strcat(message, buffer);
            bzero(buffer, 255);
            strcat(buffer, "Client with turn ");
            char c[2] = {turn_counter + '0', '\0'};
            strcat(buffer, c);
            if(message[0] == 'p'&& message[1] == 'a'&& message[2] == 's'&& message[3] == 's')
                strcat(buffer, " passed his/her turn.\n");  
            else
            {   
                if (turn_counter == 0)
                    strcat(buffer, " asked: ");   
                else
                    strcat(buffer, " answered: "); 
                strcat(buffer, message);
            }
            sendto(udp_socket_fd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address_send_to, sizeof(struct sockaddr_in));
            my_turn = turn_counter;
            if(turn_counter == ROOM_CAPACITY - 1)
            {
                bzero(best_answer, 255);
                strcat(best_answer, message);
            }
        }
        if (FD_ISSET(udp_socket_fd, &cur_socket)) 
        {
            socklen_t bc_address_size = sizeof(bc_address_recvie_from);
            bzero(buffer, 255);
            if (recvfrom(udp_socket_fd, buffer, 255, 0, (struct sockaddr *)&bc_address_recvie_from, &bc_address_size) < 0)
            {
                write(STDERR,"ERROR: reading broadcasted message failed\n", strlen("ERROR: reading broadcasted message failed\n"));
                exit(0);
            }    
            bzero(print_str, 255);
            strcat(print_str, "BROADCASTED MESSAGE: ");
            strcat(print_str, buffer);
            strcat(print_str, "\n");
            write(STDIN, print_str, strlen(print_str));
            turn_counter++;
            if(turn_counter == ROOM_CAPACITY)
                strcat(all_messages, "*");
            strcat(all_messages, buffer);
            if(buffer[19] != 'p') 
                for(int i = 29; i < strlen(buffer); i++)
                    best_answer[i - 29] = buffer[i];       
            fflush(stdout);
        }
        alarm(TIMER);
        if (turn_counter == ROOM_CAPACITY)
            break;
    }
    bzero(print_str, 255);
    strcat(print_str, "The best answer is: ");
    strcat(print_str, best_answer);
    strcat(print_str, "\n");
    write(STDIN, print_str, strlen(print_str));
    if (my_turn == 0) 
    {
        bzero(buffer, 255);
        strcat(buffer, "\nAll Q&A is:\n");
        strcat(buffer, all_messages);
        write(socket_fd, buffer, strlen(buffer));
    }
    return 0;
}
