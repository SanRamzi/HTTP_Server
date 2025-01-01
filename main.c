#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define SERVER_ADDRESS "127.0.0.1"
#define default_buflen 1024

//Usage error message
void usage_message(char* name){
    printf("Usage: %s <port_number>\n",name);
}
//Function to read a file into a char buffer
char* read_file(const char* filename,size_t* file_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("%s File open error.\n",filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(*file_size + 1);
    fread(buffer, 1, *file_size, file);
    buffer[*file_size] = '\0';  // Null-terminate the string
    fclose(file);
    return buffer;
}
//Client accept function
void* client_function(void* arg){
    int client_socket = *(int *)arg;
    free(arg);
    char read_buffer[1024];
    int bytes_sent,bytes_received = recv(client_socket,read_buffer,sizeof(read_buffer),0);
    char* method = strtok(read_buffer," ");
    char* requested_path = strtok(NULL," ");
    if(requested_path == NULL || strcmp(requested_path,"/") == 0){
        requested_path = "/index.html";
    }
    requested_path++;
    size_t file_size;
    char* response;
    char* file_content = read_file(requested_path,&file_size);
    if(file_content != NULL){
        response = malloc(file_size + 128);
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", file_size);
        bytes_sent = send(client_socket,response,strlen(response),0);
        if(bytes_sent <= 0){
            printf("Reply failed.\n");
            free(response);
            free(file_content);
            close(client_socket);
            return NULL;
        }
    bytes_sent = send(client_socket,file_content,file_size,0);
    free(response);
    free(file_content);
    }
    else{
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        bytes_sent = send(client_socket, response, strlen(response), 0);
    }
    if(bytes_sent <= 0){
        printf("Reply failed.\n");
    }
    close(client_socket);
}
int main(int argc, char *argv[]){
    //Getting port number from command line
    if(argc != 2){
        usage_message(argv[0]);
        return -1;
    }
    int port_no;
    port_no = atoi(argv[1]);
    if(port_no == 0){
        usage_message(argv[0]);
        return -1;
    }
    //Server init
    int server_socket,client_socket;
    struct sockaddr_in server;
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket < 0){
        printf("Socket creation error. Exiting...\n");
        return -1;
    }
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons(port_no);
    if(bind(server_socket,(struct sockaddr *)&server,sizeof(server))){
        close(server_socket);
        printf("Binding error. Exiting...\n");
        return -1;
    }
    if(listen(server_socket,3)){
        close(server_socket);
        printf("Listening error. Exiting...\n");
        return -1;
    }
    printf("Server started listening on %s:%d\n",SERVER_ADDRESS,port_no);
    while(1){
        struct sockaddr_in client;
        int address_size = sizeof(server);
        pthread_t child;
        int* client_socket_ptr = malloc(sizeof(int));
        client_socket = accept(server_socket,(struct sockaddr*)&client,&address_size);
        if(client_socket < 0){
            continue;
        }
        printf("Incoming connection from %s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
        *client_socket_ptr = client_socket;
        if(pthread_create(&child,NULL,client_function,client_socket_ptr)){
            printf("Error creating thread.\n");
            free(client_socket_ptr);
            continue;
        }
        else{
            pthread_detach(child);
        }
    }
    return 0;
}