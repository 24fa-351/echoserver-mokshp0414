// my port 12345 says address in use but when i use 12346 it works.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 1024

// manage the communication with the client
void *process_client(void *socket_ptr)
{
    int connection_socket = *(int *)socket_ptr;
    free(socket_ptr);
    char message[MAX_MESSAGE_LENGTH];
    ssize_t received_length;

    // go through the recieved data and send it back to the client
    while ((received_length = recv(connection_socket, message, MAX_MESSAGE_LENGTH - 1, 0)) > 0)
    {
        message[received_length] = '\0';
        printf("Received: %s", message);
        send(connection_socket, message, received_length, 0);

    if (received_length == 0)
    {
        printf("Client has disconnected.\n");
    }
    else if (received_length < 0)
    {
        perror("Failed to receive data");
    }

    // close the connection to the client
    close(connection_socket);
    return NULL;
}

void run_echo_server(int listen_port)
{
    int server_socket;
    struct sockaddr_in server_address;

    // the tcp server socket is being generated
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // the structure for the serverr
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(listen_port);

    // server address being assigned to the socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Socket binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1)
    {
        perror("Failed to listen on socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is now active on port %d...\n", listen_port);

    while (1)
    {
        struct sockaddr_in client_address;
        socklen_t address_length = sizeof(client_address);

        // new client connection needs to be accepteded
        int *client_socket = malloc(sizeof(int));
        if ((*client_socket = accept(server_socket, (struct sockaddr *)&client_address, &address_length)) == -1)
        {
            perror("Failed to accept client connection");
            free(client_socket);
            continue;
        }

        printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // thread needs to be created in order to handle the new client
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, process_client, client_socket) != 0)
        {
            perror("Failed to create client thread");
            close(*client_socket);
            free(client_socket);
            continue;
        }

        pthread_detach(client_thread);
    }

    // turnoff the server socket
    close(server_socket);
}

int main(int argc, char *argv[])
{
    if (argc != 3 || strcmp(argv[1], "-p") != 0)
    {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port_number = atoi(argv[2]);
    if (port_number <= 0 || port_number > 65535)
    {
        fprintf(stderr, "Please specify a valid port number between 1 and 65535\n");
        return EXIT_FAILURE;
    }

    // run the server
    run_echo_server(port_number);

    return 0;
}
