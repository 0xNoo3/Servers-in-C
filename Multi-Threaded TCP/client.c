#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define SERVER_NAME_LEN_MAX 255
#define BUFFER_SIZE 256

void to_lowercase(char *str);

int main(int argc, char **argv)
{
    char server_name[SERVER_NAME_LEN_MAX + 1] = {0};
    int server_port, socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;
    char message[BUFFER_SIZE];

    /* Get server name from command line arguments or stdin. */
    if (argc > 1)
    {
        strncpy(server_name, argv[1], SERVER_NAME_LEN_MAX);
    }
    else
    {
        printf("Enter Server Name: ");
        if (fgets(server_name, sizeof(server_name), stdin) == NULL)
        {
            fprintf(stderr, "Error reading server name.\n");
            exit(EXIT_FAILURE);
        }
        server_name[strcspn(server_name, "\n")] = '\0'; // Remove trailing newline
    }

    /* Get server port from command line arguments or stdin. */
    server_port = argc > 2 ? atoi(argv[2]) : 0;
    if (!server_port)
    {
        char port_str[10];
        printf("Enter Port: ");
        if (fgets(port_str, sizeof(port_str), stdin) == NULL)
        {
            fprintf(stderr, "Error reading port.\n");
            exit(EXIT_FAILURE);
        }

        server_port = atoi(port_str);
        if (!server_port)
        {
            fprintf(stderr, "Invalid port number.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Get server host from server name. */
    server_host = gethostbyname(server_name);
    if (server_host == NULL)
    {
        fprintf(stderr, "Error: no such host: %s\n", server_name);
        exit(EXIT_FAILURE);
    }

    /* Initialise IPv4 server address with server host. */
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Pending Connection Request with PID : %d\n", getpid());

    /* Connect to server using the server address. */
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1)
    {
        perror("connect");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    char pid[16];
    sprintf(pid, "%d", getpid());
    if (write(socket_fd, pid, strlen(pid)) < 0)
    {
        perror("writing PID of Client\n");
    }

    printf("Connected to server %s:%d\n", server_name, server_port);

    // while (1) // we just want one communication with server if we wanna chat just uncomment this
    {
        /* Get user input with spaces */
        printf("Enter message: ");
        if (fgets(message, sizeof(message), stdin) == NULL)
        {
            fprintf(stderr, "Error reading message from stdin.\n");
            // break;
        }
        /* Remove newline character if present */

        message[strcspn(message, "\n")] = '\0';

        to_lowercase(message);

        printf("Connection Request Sent with PID : %d : String: %s \n", getpid(), message);

        /* Write message to server */
        if (write(socket_fd, message, strlen(message)) < 0)
        {
            perror("write msg");
            // break;
        }

        /* Read response from server */
        memset(message, 0, sizeof(message));
        if (read(socket_fd, message, sizeof(message) - 1) < 0)
        {
            perror("read msg");
            // break;
        }
        printf("Message recieved from the server : %s\n", message);
    }

    close(socket_fd);
    return 0;
}

void to_lowercase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}
