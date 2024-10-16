#include "headers/chat.h"

/**
 * Processes commands received from a client.
 * 
 * @param index Index of the client.
 * @param msg Command message received from the client.
 * @param server Pointer to the ChatServer structure.
 */
int processCommand(const char* msg, ChatServer* server) {
    char* msgCopy = (char*)calloc(strlen(msg) + 1, sizeof(char));
    strcpy(msgCopy, msg);

    pthread_mutex_lock(&server->lock);
    char* msgTok = strtok(msgCopy, " ");
    pthread_mutex_unlock(&server->lock);

    if (strcmp(msgTok, "/commands\n") == 0) {free(msgCopy); msgCopy = NULL; return 0;}
    else if (strcmp(msgTok, "/members\n") == 0) {free(msgCopy); msgCopy = NULL; return 1;}
    else if (strcmp(msgTok, "/whisper") == 0) {free(msgCopy); msgCopy = NULL; return 2;}
    else if (strcmp(msgTok, "/kick") == 0) {free(msgCopy); msgCopy = NULL; return 3;}
    else if (strcmp(msgTok, "/bye\n") == 0) {free(msgCopy); msgCopy = NULL; return 4;}
    else if (strcmp(msgTok, "/sendFile\n") == 0) {free(msgCopy); msgCopy = NULL; return 5;}
    else if (strcmp(msgTok, "/recvFile\n") == 0) {free(msgCopy); msgCopy = NULL; return 6;}
    return -1; 
}

/**
 * Lists all commands to the requesting client.
 * 
 * @param index Index of the requesting client.
 * @param server Pointer to the ChatServer structure.
 */
void listCommands(int index, ChatServer* server) {
    char* list = "Commands :\n\t/commands : list all the commands\n\t/members : list all the members in chat\n\t/whisper <username> <message> : send a private message to someone\n\t/kick <username> : kick someone\n\t/sendFile : opens a prompt for file sending\n\t/recvFile : download file from server to local\n\t/bye : exit from chat\n";
    sendMessage(index, "", list, server); 
}

/**
 * Lists all connected clients to the requesting client.
 * 
 * @param index Index of the requesting client.
 * @param server Pointer to the ChatServer structure.
 */
void listClients(int index, ChatServer* server) {
    int totalLength = 0;
    for (int i = 0; i < server->max_clients; i++) {
        if (server->clients[i].username != NULL) {totalLength += strlen(server->clients[i].username) + 1;}
    }
    char* list = (char*)malloc((totalLength + 11) * sizeof(char));
    strcpy(list, "Members :\n\t");
    for (int i = 0; i < server->max_clients; i++) {
        if (server->clients[i].username != NULL) {
            strcat(list, server->clients[i].username);
            strcat(list, "\t");
        }
    }
    sendMessage(index, "", list, server);
    free(list);
    list = NULL;
}

/**
 * Sends a private message from one client to another.
 * 
 * @param index Index of the sender client.
 * @param msg Message to be sent.
 * @param server Pointer to the ChatServer structure.
 */
void privateMessage(int index, const char* msg, ChatServer* server) {
    char* msgCopy = (char*)calloc(strlen(msg)+1, sizeof(char));
    strcpy(msgCopy, msg);

    pthread_mutex_lock(&server->lock);
    char* msgTok = strtok(msgCopy, " "); // "/whisper"
    char* username = strtok(NULL, " "); // "<username>"
    msgTok = strtok (NULL, "\n"); // "<message>"
    pthread_mutex_unlock(&server->lock);

    int whisperIndex = 0;
    while (whisperIndex < server->max_clients) {
        if (whisperIndex != index && server->clients[whisperIndex].username != NULL) {
            char* usernameSliced = (char*)malloc(strlen(server->clients[whisperIndex].username));
            strcpy(usernameSliced, server->clients[whisperIndex].username);
            usernameSliced[strlen(usernameSliced)-1] = '\0';
            if (strcmp(username, usernameSliced) == 0) {break;}
            free(usernameSliced);
            usernameSliced = NULL;
        }
        whisperIndex += 1;
    }

    if (whisperIndex < server->max_clients) {
        char* msg = (char*)malloc((strlen(msgTok) + 13)*sizeof(char));
        strcpy(msg, msgTok);
        strcat(msg, " (whispered)\n");
        sendMessage(whisperIndex, server->clients[index].username, msg, server);
    } else {
        char* error = "User does not exist\n";
        sendMessage(index, "", error, server);
    }
    free(msgCopy);
    msgCopy = NULL;
}

/**
 * Kicks a client from server.
 * 
 * @param index Index of the sender client.
 * @param msg Message to be sent.
 * @param server Pointer to the ChatServer structure.
 */
void kickClient(int index, const char* msg, ChatServer* server) {
    char* msgCopy = (char*)calloc(strlen(msg) + 1, sizeof(char));
    strcpy(msgCopy, msg);

    pthread_mutex_lock(&server->lock);
    char* username = strtok(msgCopy, " "); // "/kick"
    username = strtok(NULL, " "); // "<username>"
    pthread_mutex_unlock(&server->lock);

    int kickIndex = 0;
    while (kickIndex < server->max_clients) {
        if (kickIndex != index && server->clients[kickIndex].username != NULL) {
            if (strcmp(username, server->clients[kickIndex].username) == 0) {break;}
        }
        kickIndex += 1;
    }

    if (kickIndex < server->max_clients) {
        removeClient(kickIndex, server);
    } else {
        char* error = "User does not exist\n";
        sendMessage(index, "", error, server);
    }
    free(msgCopy);
    msgCopy = NULL;
}