#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "serverutil.h"

char *get_ip_str(int ip, char *ip_str) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    sprintf(ip_str, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
	return ip_str;
}

void add_client(struct client *clients, int *current_clients, int sock_fd) {
    clients[*current_clients].sock_fd = sock_fd;
    *current_clients = *current_clients + 1;
}

int remove_client_by_fd(struct client *clients, int *current_clients, int sock_fd) {
    int found = 0;
    // search the client with the correct sock_fd
    for(int i = 0; i < *current_clients; i++) {
        if(clients[i].sock_fd == sock_fd) {
            close(clients[i].sock_fd); // close the connection to the client
            if(clients[i].name != NULL) {
                free(clients[i].name); // free the name string
                clients[i].name = NULL;
            }
            found = 1;
            // move all clients one to the left
            for(int j = i; j < *current_clients-1; j++) {
                clients[j] = clients[j+1];
            }
            break;
        }
    }
    
    if(!found) // no client with sock_fd was found
        return -1;

    *current_clients=*current_clients - 1; // decrease the client index
    return 0;
}

int remove_client(struct client *clients, int *current_clients, int client_ind) {
    if(client_ind < 0 || client_ind >= *current_clients) {
        return -1; // invalid client index
    }

    close(clients[client_ind].sock_fd); // close the connection to the client
    if(clients[client_ind].name != NULL) {
        free(clients[client_ind].name); // free name string
        clients[client_ind].name = NULL;
    }
    // move all the clients after this one left
    for(int i = client_ind; i < *current_clients-1; i++) {
        clients[i] = clients[i+1];
    }
    
    *current_clients=*current_clients - 1; // decrease the client index
    return 0;
}

void show_clients(struct client *clients, int current_clients) {
    if(!current_clients) {
        printf("no clients\n");
        return;
    }
    printf("id: \t name: \t sockfd:\n");
    for(int i = 0; i < current_clients; i++) {
        printf("%d \t %s \t %d\n", i, clients[i].name, clients[i].sock_fd);
    }
}

void delete_clients(struct client *clients, int max_clients) {
    //free the names
    for(int i = 0; i < max_clients; i++) {
        if(clients[i].name != NULL) {
            free(clients[i].name);
            clients[i].name = NULL;
        }
    }

    free(clients);
}

void saybye(struct client *clients, int current_clients, char *message, size_t size) {
    for(int i = 0; i < current_clients; i++) {
        int sd = clients[i].sock_fd;
        if(sd > 0) { // valid sock_fd
            send(sd, message, size, 0);
        }
    }
}

char *get_name(struct client *clients, int *current_clients, int sock_fd) {
    for(int i = 0; i < *current_clients; i++) {
        if(sock_fd == clients[i].sock_fd) {
            return clients[i].name;
        }
    }

    return NULL;
}

int set_name(struct client *clients, int client_ind, char *name, size_t size) {
    char *temp = (char *)malloc(size);
    if(temp == NULL)
        return -1;
    strncpy(temp, name, size);
    clients[client_ind].name = temp;
    return 0;
}


char *myfgets(char *str, size_t len, FILE *file) {
    char *result = fgets(str, sizeof(str), file);
    for(int i = 0; i < len; i++) {
        if(str[i] == '\n') {
            str[i] = '\0';
            break;
        }
    }
    return result;
}
