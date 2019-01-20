//gcc server.c -o server.o -lpthread
//./server.o
//first 6 tabs have to be opened with "Chrome" or "Opera" browser after 6 tabs, remaining tabs have to be opened
//with the other browser that you did not use for first 6 tabs, 
//i don't know why but 10 or more tabs can be opened only like this 

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <pthread.h>   //for threading , link with lpthread

#define MAX_SIZE 500
#define MAX_CLIENT 10
#define PORT 6789
#define IP "127.0.0.1"

int clientCount = 0;
pthread_mutex_t lock;

void *connection_handler(void *);
char *response(char *payload);
char *request(char *clientRequest);
char *readFile(char *fileName);

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(PORT);

    //Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, 20);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    pthread_t thread_id;
    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        clientCount++;
        printf("Number Of Connected Client %d\n", clientCount);

        puts("Connection accepted");

        if (pthread_create(&thread_id, NULL, connection_handler, (void *)&client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        puts("Handler assigned");
    }

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    pthread_mutex_destroy(&lock);

    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int *)socket_desc;
    int read_size;
    char *message, client_message[MAX_SIZE];

    while ((read_size = recv(sock, client_message, MAX_SIZE, 0)) > 0)
    {
        char *requestUrl = request(client_message);
        char *tempRequestUrl = malloc(strlen(requestUrl) + 1);
        strcpy(tempRequestUrl, requestUrl);

        char *payload = malloc(500);

        if (strstr(tempRequestUrl, ".") != NULL)
        {
            char *urlType = strtok(tempRequestUrl, ".");
            urlType = strtok(NULL, ".");
            if (strcmp(urlType, "html") == 0 || strcmp(urlType, "jpeg") == 0 || strcmp(urlType, "jpg") == 0)
            {
                payload = readFile(requestUrl);
            }
            else
            {
                strcpy(payload, "BAD REQUEST");
            }
        }
        else
            strcpy(payload, "BAD REQUEST");

        char *responseMessage = response(payload);
        send(sock, responseMessage, strlen(responseMessage), 0);

        client_message[read_size] = '\0';
        memset(client_message, 0, MAX_SIZE);
    }

    if (read_size == 0)
    {
        pthread_mutex_lock(&lock);
        clientCount--;
        pthread_mutex_unlock(&lock);

        puts("Client disconnected");
    }
    else if (read_size == -1)
    {
        pthread_mutex_lock(&lock);
        clientCount--;
        pthread_mutex_unlock(&lock);

        perror("recv failed");
    }

    return 0;
}

char *request(char *clientRequest)
{
    char *first_line_of_header = strtok(clientRequest, "\n");
    char *url = strtok(first_line_of_header, " ");
    url = strtok(NULL, " ");
    // remove / character
    url = &url[1];
    return url;
}

char *response(char *payload)
{
    char *response_code = malloc(30);
    char *response_content_length = malloc(30);

    int content_length = 0;
    char *response_payload = malloc(MAX_SIZE);

    if (clientCount < 11)
    {
        puts("response gidecek");
        if (payload != NULL && strcmp(payload, "BAD REQUEST") != 0)
        {
            sprintf(response_code, "HTTP/1.0 %d OK\n", 200);
            content_length = strlen(payload) + 1;
            strcpy(response_payload, payload);
        }
        else if (payload == NULL)
        {
            puts("not found bloğu");
            sprintf(response_code, "HTTP/1.0 %d Not Found\n", 404);
            strcpy(response_payload, "");
        }
        else
        {
            puts("ivalid found bloğu");
            sprintf(response_code, "HTTP/1.0 %d Bad Request\n", 400);
            strcpy(response_payload, "");
        }
    }
    else
    {
        sprintf(response_code, "HTTP/1.0 %d OK\n", 200);
        if (payload != NULL)
            content_length = strlen(payload) + 1;

        strcpy(response_payload, "<p>Server Is Busy</p>");
        //.img falan olmayacak
        //clientCount 10 u geçtikten sonra yanlış html açmada hata
    }

    sprintf(response_content_length, "Content-Length: %d\n", content_length);

    char *response = malloc(MAX_SIZE);
    char *response_header = malloc(200);

    strcpy(response_header, "Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
                            "Server: Apache/1.3.0\n"
                            "Content-Type: text/html\n");

    strcpy(response, response_code);
    strcat(response, response_header);
    strcat(response, response_content_length);
    strcat(response, "Connection: close\n\n");
    strcat(response, response_payload);

    return response;
}

char *readFile(char *fileName)
{
    char *buffer = 0;
    long length;
    FILE *f = fopen(fileName, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer)
        {
            fread(buffer, 1, length, f);
        }
        fclose(f);
        return buffer;
    }
    return NULL;
}
