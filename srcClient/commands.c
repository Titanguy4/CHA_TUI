#include "headers/chat.h"

/**
 * @brief Check if the message is a command
 * 
 * @param msg the message to check
*/
int checkCommand(char* msg) {
	char* msgCopy = (char*)calloc(strlen(msg)+1, sizeof(char));
	strcpy(msgCopy, msg);
    char* msgTok = strtok(msgCopy, " ");

	if (strcmp(msgTok, "/sendFile\n") == 0) {free(msgCopy); return 0;}
    else if (strcmp(msgTok, "/recvFile\n") == 0) {free(msgCopy); return 1;}
	return -1;
}