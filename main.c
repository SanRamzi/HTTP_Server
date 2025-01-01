#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_ADDRESS "127.0.0.1"
#define default_buflen 1024

//Usage error message
void usage_message(char* name){
    printf("Usage: %s <port_number>\n",name);
}
//Client accept function
void* client(void* arg){
    int client_socket = *(int *)arg;
    write(client_socket,"User connected.\n",17);
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
        int address_size = sizeof(server);
        pthread_t child;
        client_socket = accept(server_socket,(struct sockaddr*)&server,&address_size);
        if(!client_socket){
            continue;
        }
        printf("Incoming connection from %s:%d\n",inet_ntoa(server.sin_addr),ntohs(server.sin_port));
        if(pthread_create(&child,NULL,client,&client_socket)){
            printf("Error creating thread.\n");
            continue;
        }
        else{
            pthread_detach(child);
        }
    }
    return 0;
}