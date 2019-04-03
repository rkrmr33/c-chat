#ifndef SERVUTIL_H   /* Include guard */
#define SERVUTIL_H

#define CLIENT_NAME_SIZE 10

struct client {
    char *name;
    int sock_fd;
};

char *get_ip_str(int ip, char *ip_str);
void add_client(struct client *clients, int *current_clients, int sock_fd);
int remove_client_by_fd(struct client *clients, int *current_clients, int sock_fd);
void delete_clients(struct client *clients, int max_clients);
int remove_client(struct client *clients, int *current_clients, int client_ind);
void show_clients(struct client *clients, int current_clients);
void saybye(struct client *clients, int current_clients, char *message, size_t size);
char *myfgets(char *str, size_t len, FILE *file);
char *get_name_from_client(int sock_fd);
char *get_name(struct client *clients, int *current_clients, int sock_fd);
int set_name(struct client *clients, int client_ind, char *name, size_t size);

#endif // SERVUTIL_H