#include "headers/chat.h"

/**
 * Accepts a new client connection for file transfer.
 * 
 * @param server Pointer to the ChatServer structure.
 * @return descriptor of new client or -1 on failure.
 */
int acceptFileConnection(ChatServer* server) {

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_socket;

    // Accept new chat connection
    new_socket = accept(server->file_server_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (new_socket < 0) {
        perror("Accepting new file client failed");
        return -1;
    }

    printf("A Client connected to file server\n");
    return new_socket;
}

/**
 * Receives a message from a file client.
 * 
 * @param dS socket descriptor of file client.
 * @param msg Pointer to the buffer to store the received message.
 * @return Length of the received message or -1 on failure.
 */
int receiveFileMessage(int dS, char** msg) {
    int msgLength;
    if (recv(dS, &msgLength, sizeof(int), 0) <= 0) {return -1;}
    char* msgRecv = (char*)calloc(msgLength, sizeof(char));
    if (recv(dS, msgRecv, msgLength*sizeof(char), 0) <= 0) {free(msgRecv); msgRecv = NULL; return -1;}

    *msg = msgRecv;
    return msgLength;
}

/**
 * Sends a message to a file client.
 * 
 * @param dS socket descriptor of file client.
 * @param msg Message to send.
 */
void* receiveFileThread(void* args) {
    ChatServer* server = (ChatServer*)args;
    int dS = acceptFileConnection(server);

    char* filename;
    if (receiveFileMessage(dS, &filename) == -1) {pthread_exit(0);}

    printf("Receiving %s\n", filename);

    const char *directory = "filesServer";
    char* filepath = (char*)malloc(sizeof(char)*(strlen(directory)+strlen(filename)+2));
    snprintf(filepath, sizeof(char)*(strlen(directory)+strlen(filename)+2), "%s/%s", directory, filename);

    FILE *filePointer;
    filePointer = fopen(filepath, "wb");

    if (filePointer == NULL) {pthread_exit(0);}

    int fileLength;
    if (recv(dS, &fileLength, sizeof(int), 0) <= 0) {pthread_exit(0);}

    int wrote = 0;
    while (wrote < fileLength) {
        char* bloc;
        int blocLength = receiveFileMessage(dS, &bloc);
        fwrite(bloc, sizeof(char), blocLength, filePointer);
        wrote += blocLength;
        free(bloc);
        bloc = NULL;
    }

    printf("File %s received\n", filename);
    fclose(filePointer);
    free(filepath);
    filepath = NULL;
    free(filename);
    filename = NULL;

    shutdown(dS,2);
    pthread_exit(0);
}

/**
 * Receives a file from a client.
 * 
 * @param server Pointer to the ChatServer structure.
 */
void receiveFile(ChatServer* server) {
    pthread_t thread;
    pthread_create(&thread, 0, receiveFileThread, (void*)server);
}

void* sendFileThread(void* args) {
    ChatServer* server = (ChatServer*)args;
    int dS = acceptFileConnection(server);

    char* filename;
    if (receiveFileMessage(dS, &filename) == -1) {pthread_exit(0);}

    printf("sending %s\n", filename);

    const char *directory = "filesServer";
    char* filepath = (char*)malloc(sizeof(char)*(strlen(directory)+strlen(filename)+2));
    snprintf(filepath, sizeof(char)*(strlen(directory)+strlen(filename)+2), "%s/%s", directory, filename);

    FILE *filePointer;
    filePointer = fopen(filepath, "rb");

    if (filePointer == NULL) {pthread_exit(0);}

    fseek(filePointer, 0, SEEK_END);
    int fileLength = ftell(filePointer);
    if (send(dS, &fileLength, sizeof(int), 0) < 0) {pthread_exit(0);}
    fseek(filePointer, 0, SEEK_SET);

    const size_t chunkSize = 256;
    char buffer[chunkSize];
    
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, chunkSize, filePointer)) > 0) {
        if (send(dS, &bytesRead, sizeof(int), 0) < 0) {pthread_exit(0);}
        if (send(dS, buffer, bytesRead, 0) < 0) {pthread_exit(0);}
    }

    printf("File %s sent\n", filename);
    fclose(filePointer);
    free(filepath);
    filepath = NULL;
    free(filename);
    filename = NULL;

    shutdown(dS,2);
    pthread_exit(0);
}

/**
 * Sends a file to a client.
 * 
 * @param index Index of the client.
 * @param server Pointer to the ChatServer structure.
 */
void sendFile(int index, ChatServer* server) {
    struct dirent *entry;
    DIR *dr = opendir("filesServer"); 
  
    if (dr == NULL) { 
        printf("Could not open source directory" ); 
        return;
    } 
 
    int total = 0;
    while ((entry = readdir(dr)) != NULL) {
        if (entry->d_type == DT_REG) { /* If the entry is a regular file */
             total++;
        }
    }
    if (send(server->clients[index].chat_socket, &total, sizeof(int), 0) < 0) {return;}

    rewinddir(dr);

    while ((entry = readdir(dr)) != NULL) {
        if (entry->d_type == DT_REG) {
            int fileLength = strlen(entry->d_name) + 1;
            if (send(server->clients[index].chat_socket, &fileLength, sizeof(int), 0) < 0) {return;}
            
            char* filename = (char*)malloc(sizeof(char)*fileLength);
            strcpy(filename, entry->d_name);
            if (send(server->clients[index].chat_socket, filename, fileLength, 0) < 0) {return;}
            free(filename);
            filename = NULL;
        }
    }

    closedir(dr);

    pthread_t thread;
    pthread_create(&thread, 0, sendFileThread, (void*)server);
}