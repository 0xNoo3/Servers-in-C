#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>

int connection_count = 0;
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;

void to_uppercase(char *str);

#define BACKLOG 10
#define BUFFER_SIZE 256

typedef struct pthread_arg_t
{
    int new_socket_fd;
    struct sockaddr_in client_address;
    /* Additional arguments for threads can be added here */
} pthread_arg_t;

/* Global socket file descriptor for cleanup in signal handler */
int socket_fd_global;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

int main(int argc, char **argv)
{
    int port, new_socket_fd;
    struct sockaddr_in address;
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;

    /* Get port from command line arguments or stdin. */
    port = argc > 1 ? atoi(argv[1]) : 0;
    if (!port)
    {
        printf("Enter Port: ");
        if (scanf("%d", &port) != 1)
        {
            fprintf(stderr, "Invalid port number.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Initialise IPv4 address. */
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    /* Create TCP socket. */
    if ((socket_fd_global = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Bind address to socket. */
    if (bind(socket_fd_global, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind");
        close(socket_fd_global);
        exit(EXIT_FAILURE);
    }

    /* Listen on socket. */
    if (listen(socket_fd_global, BACKLOG) == -1)
    {
        perror("listen");
        close(socket_fd_global);
        exit(EXIT_FAILURE);
    }


    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_attr) != 0)
    {
        perror("pthread_attr_init");
        exit(EXIT_FAILURE);
    }
    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        perror("pthread_attr_setdetachstate");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", port);

    while (1)
    {
        /* Allocate pthread argument for each connection to client. */
        pthread_arg = malloc(sizeof(*pthread_arg));
        if (!pthread_arg)
        {
            perror("malloc");
            continue;
        }

        /* Accept connection to client. */
        if (connection_count < 3) // Max 3 connections
        {
            sleep(0.5);
            printf("Waiting for new connection requests...\n");
        }
        client_address_len = sizeof(pthread_arg->client_address);
        new_socket_fd = accept(socket_fd_global, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        if (new_socket_fd == -1)
        {
            perror("accept");
            free(pthread_arg);
            continue;
        }

        pthread_arg->new_socket_fd = new_socket_fd;

        /* Check for maximum connection limit */
        pthread_mutex_lock(&conn_mutex);
        if (connection_count >= 3)
        {
            char pid_Client[16];
            pthread_mutex_unlock(&conn_mutex); 
            fprintf(stderr, "Maximum connections reached. Rejecting connection.\n");
            shutdown(new_socket_fd, SHUT_RDWR);
            close(new_socket_fd);
            free(pthread_arg);
            continue;
        }
        connection_count++;
        pthread_mutex_unlock(&conn_mutex);

        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0)
        {
            perror("pthread_create");
            close(new_socket_fd);
            free(pthread_arg);
            /* Decrement count if thread creation fails */
            pthread_mutex_lock(&conn_mutex);
            connection_count--;
            pthread_mutex_unlock(&conn_mutex);

            continue;
        }
    }

    /* This point is never reached; cleanup should be done in signal_handler() */
    close(socket_fd_global);
    return 0;
}

void *pthread_routine(void *arg)
{
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    /* Free the argument memory as it is no longer needed */
    free(arg);
    char pid_Client[16];
    if (read(new_socket_fd, pid_Client, 16) < 0)
    {
        perror("read");
    }

    printf("New client connected with Pid : %s\nattached to server thread id: %lu\n", pid_Client, (unsigned long)pthread_self());

    while (1)
    {
        /* Clear buffer before use */
        memset(buffer, 0, BUFFER_SIZE);

        /* Read from client the message */
        bytes_read = read(new_socket_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0)
        {
            perror("read");
            break;
        }
        else if (bytes_read == 0)
        {
            printf("Client with PID :%s disconnected.\n", pid_Client);
            break;
        }

        printf("Client PID:%s | Connected with Data:  %s\n", pid_Client, buffer);

        to_uppercase(buffer);

        /* Write message to client */
        bytes_written = write(new_socket_fd, buffer, strlen(buffer));
        if (bytes_written < 0)
        {
            perror("write");
            break;
        }
    }

    close(new_socket_fd);
    return NULL;
}

void to_uppercase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = toupper((unsigned char)str[i]);
    }
}
