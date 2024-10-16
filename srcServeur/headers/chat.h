#ifndef SERVEUR_CHAT
#define SERVEUR_CHAT

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <dirent.h>

/* User structure to hold information about each connected client */
typedef struct {
    int chat_socket;
    char* username;
    int salon;
    pthread_t thread;
} User;

/* Chat server structure to hold server data */
typedef struct {
    int server_socket;
    int file_server_socket;
    int max_clients;
    int nb_salons;
    pthread_mutex_t lock;
    User* clients;
} ChatServer;

/* connexion.c */
int initChatServer(int max_clients, int chat_port, int file_port, int nb_salons, ChatServer* server);
int acceptClient(ChatServer* server);
int removeClient(int index, ChatServer* server);
int shutdownServer(ChatServer* server);

/* chat.c */
void startChatSession(int index, ChatServer* server);
void* handleClient(void* args);
int receiveMessage(int index, char** msg, ChatServer* server);
int sendMessage(int receiver, const char* username, const char* msg, ChatServer* server);
void broadcastMessage(int index, const char* msg, ChatServer* server);

/* commands.c */
int processCommand(const char* msg, ChatServer* server);
void listCommands(int index, ChatServer* server);
void listClients(int index, ChatServer* server);
void privateMessage(int index, const char* msg, ChatServer* server);
void kickClient(int index, const char* msg, ChatServer* server);

/* files.c */
int acceptFileConnection(ChatServer* server);
int receiveFileMessage(int dS, char** msg);
void* receiveFileThread(void* args);
void receiveFile(ChatServer* server);
void* sendFileThread(void* args);
void sendFile(int index, ChatServer* server);

#endif