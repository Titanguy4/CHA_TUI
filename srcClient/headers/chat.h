#ifndef CLIENT_CHAT
#define CLIENT_CHAT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct {
	int dS;
    char* address;
	int tailleMess;
    int portFileServer;
    pthread_t tsaisie;
    pthread_t treception;
} chat_args;

/* chat.c */
int countLines(const char* msg);
int recvMsgLength(chat_args *args);
int recvMsg(chat_args *args, int messageLength, char** messageRecu);
void display(const char* username, const char* msg);
int sendMessage(chat_args *args, char* message);
int askUsername(chat_args *args);
void* reception(void* t);
void* saisie(void* t);
void launchChat(chat_args* args);

/* commands.c */
int checkCommand(char* msg);


/* connexion.c */
void createChat(chat_args* args, char* address, int port, int portFile);
void shutdownClient(chat_args* args);

/* files.c */
int connectFileServer(chat_args* args);
void* sendFileThread(void* t);
void* receiveFileThread(void* t);
int receiveFileMessage(int dS, char** msg);
void sendFile(chat_args* args);
void recvFile(chat_args* args);


#endif