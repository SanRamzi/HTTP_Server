/*HTTP Server written in C by San Ramzi*/
/* gcc main.c -lpthread */

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

// Server Address
#define SERVER_ADDRESS "127.0.0.1"

// Usage error message
void usage_message(char *name)
{
    printf("Usage: %s <port_number>\n", name);
}

// Function to read a file into a char buffer
char *read_file(const char *filename, size_t *file_size)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(*file_size + 1);
    fread(buffer, 1, *file_size, file);
    buffer[*file_size] = '\0'; // Null-terminate the string
    fclose(file);
    return buffer;
}

// Monitor input for "exit" or "quit" to shut down server
void *monitor_input(void *arg)
{
    int server_socket = *(int *)arg;
    char input[100];
    while (1)
    {
        // Receive input from input stream
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        // If input is "exit" or "quit"
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0)
        {

            // Log server shutdown
            char shutdown_log[100];
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char time_string[100];
            snprintf(time_string, 100, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            snprintf(shutdown_log, sizeof(time_string) + 30, "%s : Shutting down server...\n\n", time_string);
            printf("%s", shutdown_log);
            FILE *log_file = fopen("server_logs", "a");
            fprintf(log_file, "%s", shutdown_log);
            fclose(log_file);

            // Close server socket and exit program
            close(server_socket);
            exit(0);
        }
    }
    return NULL;
}

// Get content-type for HTTP header
char *get_mime_type(const char *filename)
{
    const char *ext = strrchr(filename, '.'); // Get the file extension
    if (!ext)
        return "application/octet-stream"; // Default binary type if no extension
    if (strcmp(ext, ".html") == 0)
        return "text/html"; // HTML
    if (strcmp(ext, ".css") == 0)
        return "text/css"; // CSS
    if (strcmp(ext, ".js") == 0)
        return "application/javascript"; // Javascript
    if (strcmp(ext, ".json") == 0)
        return "application/json"; // JSON
    if (strcmp(ext, ".png") == 0)
        return "image/png"; // PNG image
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg"; // JPEG image
    if (strcmp(ext, ".gif") == 0)
        return "image/gif"; // GIF image
    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml"; // SVG image
    if (strcmp(ext, ".txt") == 0)
        return "text/plain"; // Plain text
    if (strcmp(ext, ".xml") == 0)
        return "application/xml"; // XML document
    if (strcmp(ext, ".pdf") == 0)
        return "application/pdf";      // PDF document
    return "application/octet-stream"; // Default for unknown types
}

// Client accept function
void *client_function(void *arg)
{

    // Get client socket from function argument
    int client_socket = *(int *)arg;
    free(arg);

    // Read HTTP request from client (1KB)
    char read_buffer[1024];
    int bytes_sent, bytes_received = recv(client_socket, read_buffer, sizeof(read_buffer), 0);

    // If bytes_received < 0 there's an error
    if (bytes_received < 0)
    {
        printf("Error receiving data...\n");
        close(client_socket);
        return NULL;
    }

    // If bytes_received = 0 the client disconnected
    if (bytes_received == 0)
    {
        printf("Client disconnected.\n");
        close(client_socket);
        return NULL;
    }

    // Get method (GET/POST) and requested path of file from read buffer
    char *method = strtok(read_buffer, " ");
    char *requested_path = strtok(NULL, " ");

    // Log client request
    char client_request_log[100];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_string[100];
    snprintf(time_string, 100, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    snprintf(client_request_log, sizeof(time_string) + sizeof(requested_path) + 26, "%s : Client requested %s\n", time_string, requested_path);
    printf("%s", client_request_log);
    FILE *log_file = fopen("server_logs", "a");
    fprintf(log_file, "%s", client_request_log);
    fclose(log_file);

    // If no path is requested respond with index.html
    if (requested_path == NULL)
    {
        strcat(requested_path, "/index.html");
    }
    if (strcmp(requested_path, "/") == 0)
    {
        strcat(requested_path, "index.html");
    }
    requested_path++;

    // Read file content into char buffer
    size_t file_size;
    char *response;
    char *file_content = read_file(requested_path, &file_size);

    // If file can be read then return file contents to client
    if (file_content != NULL)
    {
        printf("Client requested %s.\n", requested_path);
        response = malloc(file_size + 256);
        if (!response)
        {
            printf("ERROR: Memory allocation failed for response.\n");
            free(file_content);
            close(client_socket);
            return NULL;
        }

        // Send HTTP header
        char *mime_type = get_mime_type(requested_path);
        snprintf(response, file_size + 256, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", mime_type, file_size);
        bytes_sent = send(client_socket, response, strlen(response), 0);
        if (bytes_sent <= 0)
        {
            printf("Error sending response headers.\n");
            free(response);
            free(file_content);
            close(client_socket);
            return NULL;
        }

        bytes_sent = send(client_socket, file_content, file_size, 0);
        if (bytes_sent <= 0)
        {
            printf("Error sending file content.\n");
        }

        free(response);
        free(file_content);
    }
    else
    {
        // Check for filename.html
        strcat(requested_path, ".html");
        file_content = read_file(requested_path, &file_size);
        if (file_content != NULL)
        {
            response = malloc(file_size + 256);
            if (!response)
            {
                printf("ERROR: Memory allocation failed for response.\n");
                free(file_content);
                close(client_socket);
                return NULL;
            }

            // Send HTTP header
            char *mime_type = get_mime_type(requested_path);
            snprintf(response, file_size + 256, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", mime_type, file_size);

            bytes_sent = send(client_socket, response, strlen(response), 0);
            if (bytes_sent <= 0)
            {
                printf("Error sending response headers.\n");
                free(response);
                free(file_content);
                close(client_socket);
                return NULL;
            }

            bytes_sent = send(client_socket, file_content, file_size, 0);
            if (bytes_sent <= 0)
            {
                printf("Error sending file content.\n");
            }

            free(response);
            free(file_content);
        }

        // If nothing found, then return 404 error
        else
        {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            bytes_sent = send(client_socket, response, strlen(response), 0);
            if (bytes_sent < 0)
            {
                printf("Error sending 404 response.\n");
            }
        }
    }

    close(client_socket);
}

int main(int argc, char *argv[])
{

    // Getting port number from command line
    if (argc != 2)
    {
        usage_message(argv[0]);
        return -1;
    }
    int port_no;
    port_no = atoi(argv[1]);
    if (port_no == 0)
    {
        usage_message(argv[0]);
        return -1;
    }

    // Server init
    int server_socket, client_socket;
    struct sockaddr_in server;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Socket creation error
    if (server_socket < 0)
    {
        printf("Socket creation error. Exiting...\n");
        return -1;
    }

    // Set server address and port
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons(port_no);

    // Binding error
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)))
    {
        close(server_socket);
        printf("Binding error. Port might be blocked. Exiting...\n");
        return -1;
    }

    // Listening error
    if (listen(server_socket, 3))
    {
        close(server_socket);
        printf("Listening error. Exiting...\n");
        return -1;
    }

    // Log server start time
    char server_init_log[100];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_string[100];
    snprintf(time_string, 100, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    snprintf(server_init_log, sizeof(SERVER_ADDRESS) + sizeof(port_no) + sizeof(time_string) + 40, "%s : Server started listening on %s:%d\n", time_string, SERVER_ADDRESS, port_no);
    printf("%s", server_init_log);
    FILE *log_file = fopen("server_logs", "a");
    fprintf(log_file, "%s", server_init_log);
    fclose(log_file);

    // Start input thread to listen for server shutdown
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, monitor_input, &server_socket);

    // Main loop for receiving connections
    while (1)
    {
        // Set client socket, address size and new child thread
        struct sockaddr_in client;
        int address_size = sizeof(server);
        pthread_t child;
        int *client_socket_ptr = malloc(sizeof(int));

        // Accept incoming connection
        client_socket = accept(server_socket, (struct sockaddr *)&client, &address_size);
        if (client_socket < 0)
        {
            continue;
        }
        printf("Incoming request from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        *client_socket_ptr = client_socket;

        // Create a thread and run client_function
        if (pthread_create(&child, NULL, client_function, client_socket_ptr))
        {
            printf("Error creating thread.\n");
            free(client_socket_ptr);
            continue;
        }
        else
        {
            pthread_detach(child);
        }
    }
    // Shutdown server
    pthread_join(input_thread, NULL);
    printf("Server shut down.\n");
    return 0;
}