#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 9002
#define SERVER_ADDR "127.0.0.1"
#define CLIENT_NAME_SIZE 10

char *myfgets(char *str, size_t len, FILE *file) {
    char *result = fgets(str, len, file);
    for(int i = 0; i < len; i++) {
        if(str[i] == '\n') {
            str[i] = '\0';
            break;
        }
    }
    return result;
}

int main() {
    printf("--- Client ---\n");

    char client_name[CLIENT_NAME_SIZE];
    int client_socket, status_chk, max_fd, fdready_count, read_count;
    struct sockaddr_in server_address;
    fd_set readset;

    // get client name
    do {
        printf("please enter username (10 characters max, min 3): ");
        myfgets(client_name, sizeof(client_name), stdin);
    } while(strlen(client_name) < 3 || strlen(client_name) > 10);

    // create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify server options
    server_address.sin_family = AF_INET;
    // convert address and check for errors
    if(inet_pton(AF_INET, SERVER_ADDR, &(server_address.sin_addr)) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }
    server_address.sin_port = SERVER_PORT;

    // connect the socket and check the connection status
    if((status_chk = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address))) == -1) {
        perror("connection error");
        exit(EXIT_FAILURE);
    }

    // send client name to server
    send(client_socket, client_name, strlen(client_name) + 1, 0); // +1 for \0

    // create buffer for messages
    char mesg_buff[128];
    
    while(1) {
        FD_ZERO(&readset);

        FD_SET(fileno(stdin), &readset);
        max_fd = fileno(stdin);

        FD_SET(client_socket, &readset);
        if(client_socket > max_fd)
            max_fd = client_socket;

        fdready_count = select(max_fd + 1, &readset, NULL, NULL, NULL);

        // stdin is ready : send message
        if(FD_ISSET(fileno(stdin), &readset)) {
            myfgets(mesg_buff, sizeof(mesg_buff), stdin);
            if(strlen(mesg_buff) != 0) {
                send(client_socket, mesg_buff, sizeof(mesg_buff), 0);
                printf("%s: %s\n", client_name, mesg_buff);
            }
        }

        // socket is ready : receive new message from server
        if(FD_ISSET(client_socket, &readset)) {
            // if read bytes is zero than the connection was terminated
            if((read_count = read(client_socket, mesg_buff, sizeof(mesg_buff))) == 0) {
                printf("The connection to the server was closed.\n");
                break;
            } else {
                printf("%s\n", mesg_buff);
            }
        }

    }

   
    // close the connection
    close(client_socket);
    printf("closing connection, bye!\n");

    return 0;
}


