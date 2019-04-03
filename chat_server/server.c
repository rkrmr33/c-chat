#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "serverutil.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 9002
#define SERVER_ADDR "192.168.1.21"
#define DEFAULT_MAX_CLIENTS 10

char mesg_server_full[] = "sorry, the server is full. bye!";
char mesg_greeting[] = "server: Welcome to the server - ";
char mesg_server_closed[] = "The server has be closed. Try again later.";

int command_handler(char *command_buf, struct client *clients_list, int max_clients, int current_clients);

int main() {
    printf("--- Server ---\n");

    int server_socket, status_chk, sd;
    int max_clients, current_clients = 0, read_size, fdready_count, max_fd, addrlen;
    char command_buf[16];
    char read_buf[128], resp_buf[128];
    struct client *clients_list;
    fd_set readset;

    // get max number of clients
    printf("please enter max number of clients (max 9999): ");
    char max_clients_str[5];
    fgets(max_clients_str, sizeof(max_clients_str), stdin);
    max_clients = strtol(max_clients_str, NULL, 10);
    if(max_clients <= 0)
        max_clients = DEFAULT_MAX_CLIENTS;
    clients_list = calloc(max_clients + 1, sizeof(struct client)); // +1 for allowing last client connection and send refuse mesg
    if(clients_list == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }

    // create server socket
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0 ) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set an option so you can restart the server immidiately 
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("socket options");
        exit(EXIT_FAILURE);
    }

    // specify server options
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = SERVER_PORT;

    // bind the socket and check the connection status
    status_chk = bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    if(status_chk == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    } else if(status_chk == 0) {
        char ip[16];
        get_ip_str(address.sin_addr.s_addr, ip);
        printf("socket bound to address: %s\n", ip);
    }

    // listen for connections
    status_chk = listen(server_socket, max_clients+1); // +1 for allowing last client connection and send refuse mesg
    if(status_chk == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    } else if(status_chk == 0) {
        printf("server lisening on port:%d\n", SERVER_PORT);
    }

    addrlen = sizeof(address);
    printf("$ ");
    fflush(stdout);
    
    int quit = 0;
    while(!quit) {
        // create fd_set
        FD_ZERO(&readset);
        // add stdin for server commands
        FD_SET(fileno(stdin), &readset);
        max_fd = fileno(stdin);

        // add the server socket
        FD_SET(server_socket, &readset);
        if(server_socket > max_fd)
            max_fd = server_socket;

        // add all client sockets
        for(int i = 0; i < current_clients; i++) {
            sd = clients_list[i].sock_fd;

            if(sd <= 0) // invalid socket descriptor
                continue;

            // add current client socket descriptor
            FD_SET(sd, &readset);

            if(sd > max_fd)
                max_fd = sd;
        }

        // update the fd read set
        fdready_count = select(max_fd + 1, &readset, NULL, NULL, NULL);

        // no ready file descriptors continue
        if(!fdready_count) 
            continue;
        else if((fdready_count < 0) && (errno != EINTR)) // select error
            perror("select");

        // stdin is ready to read : handle server commands
        if(FD_ISSET(fileno(stdin), &readset)) {
            quit = command_handler(command_buf, clients_list, max_clients, current_clients);
        }

        // the server socket is ready : new client connection
        if(FD_ISSET(server_socket, &readset)) {
            // accept new connection
            sd = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if(sd <= 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            // server is full
            if(current_clients == max_clients) { 
                send(sd, mesg_server_full, sizeof(mesg_server_full), 0); // send server_full_mesg
                continue;
            } else { // add to clients_list
                read_size = read(sd, read_buf, sizeof(read_buf));
                
                if(read_size > 0) { // got client name
                    set_name(clients_list, current_clients, read_buf, read_size);
                    printf("\n> new client joined - %s!\n", read_buf);
                    strncpy(resp_buf, mesg_greeting, sizeof(resp_buf));
                    strcat(resp_buf, read_buf);
                    send(sd, resp_buf, sizeof(resp_buf), 0);
                } else { // dont have client name
                    char ip[16];
                    get_ip_str(address.sin_addr.s_addr, ip);
                    printf("\n> new client joined - %s!\n", ip);
                    strncpy(resp_buf, mesg_greeting, sizeof(resp_buf));
                    send(sd, resp_buf, sizeof(resp_buf), 0);
                }

                add_client(clients_list, &current_clients, sd); // add the client to the clients list
                printf("$ "); // new promp
                fflush(stdout);
            }
        }

        // check all client sockets : new messages from clients
        for(int i = 0; i < current_clients; i++) {
            sd = clients_list[i].sock_fd;

            // invalid socket
            if(sd <= 0) {
                remove_client(clients_list, &current_clients, i);
                i--; // take a step back because the sockets moved one left
                continue;
            }

            // client socket is ready to read
            if(FD_ISSET(sd, &readset)) {

                // if the read bytes is 0 then the client disconnected, remove it
                if((read_size = read(sd, read_buf, sizeof(read_buf))) == 0) {
                    char *name;
                    // check if user had a name
                    if((name = get_name(clients_list, &current_clients, sd)) == NULL) {
                        printf("\n> unnamed user left.\n");
                        printf("$ "); // new promp
                        fflush(stdout);
                    } else {
                        printf("\n> %s left.\n", name);
                        printf("$ "); // new promp
                        fflush(stdout);
                    }
                    
                    remove_client_by_fd(clients_list, &current_clients, sd);
                    fflush(stdout);
                } else {
                    int left_len, curr_client;

                    // building the message response
                    if(clients_list[i].name != NULL) {
                        strcpy(resp_buf, clients_list[i].name);
                        //printf("%s :", clients_list[i].name);
                    } else {
                        strcpy(resp_buf, "anonymous");
                    }
                    left_len = sizeof(resp_buf) - strlen(resp_buf);
                    strncat(resp_buf, ": ", left_len);
                    left_len = sizeof(resp_buf) - strlen(resp_buf);
                    strncat(resp_buf, read_buf, left_len);
                    //printf("%s\n", resp_buf);

                    for(int j = 0; j < current_clients; j++) {
                        if(j != i) {
                            curr_client = clients_list[j].sock_fd;
                            send(curr_client, resp_buf, sizeof(resp_buf), 0);
                        }
                    }
                }
            }
        }

    }

    // close the connection
    printf("closing connection, bye!\n");
    close(server_socket);
    // free client list memory
    delete_clients(clients_list, max_clients + 1);

    return 0;
}

int command_handler(char *command_buf, struct client *clients_list, int max_clients, int current_clients) {
    //get new command
    myfgets(command_buf, sizeof(command_buf), stdin);
    fseek(stdin, 0, SEEK_END); // clear stdin in case of overflow
    
    // switch command and handle
    if(!strcmp(command_buf, "quit")) {
        saybye(clients_list, current_clients,mesg_server_closed, sizeof(mesg_server_closed));
        return 1;
    } else if(!strcmp(command_buf, "clients")) { // show all clients names
        show_clients(clients_list, current_clients);
    } else if(!strcmp(command_buf, "ccount")) { // show current client count
        printf("%d\n", current_clients);
    } else if(!strcmp(command_buf, "cmax")) { // show max_clients value
        printf("%d\n", max_clients);
    } else {
        printf("invalid command, try again!\n");
    }

    // print next prompt
    printf("$ ");
    fflush(stdout);
    return 0;
}

