#include "headers/chat.h"

/**
 * Starts a chat session for a connected client.
 * 
 * @param index Index of the client.
 * @param server Pointer to the ChatServer structure.
 */
void startChatSession(int index, ChatServer* server) {
	typedef struct {int index; ChatServer server;} trans_args;

	trans_args* t = (trans_args*)malloc(sizeof(trans_args));
	t->index = index;
	t->server = *server;
	pthread_create(&server->clients[index].thread, 0, handleClient, (void*)t);
}

/**
 * Handles message transmission for a client.
 * 
 * @param args Pointer to the ChatServer structure.
 * @return NULL
 */
void* handleClient(void* args) {
	typedef struct {int index; ChatServer server;} trans_args;
	trans_args* t = (trans_args*)args;

	int index = t->index;
	ChatServer server = t->server;

	char* msg;
	int res;
	do {
		if ((res = receiveMessage(index, &msg, &server)) < 0) {
			removeClient(index, &server);
			break;
		}
		if (res > 0) {
			int command;
			if ((command = processCommand(msg, &server)) < 0) {broadcastMessage(index, msg, &server);}
			else if (command == 0) {printf("LIST\n"); listCommands(index, &server);}
			else if (command == 1) {printf("MEMBERS\n"); listClients(index, &server);}
			else if (command == 2) {printf("WHISPER\n"); privateMessage(index, msg, &server);}
			else if (command == 3) {printf("KICK\n"); kickClient(index, msg, &server);}
			else if (command == 4) {printf("BYE\n"); removeClient(index, &server); break;}
			else if (command == 5) {receiveFile(&server);}
			else if (command == 6) {
				sendMessage(index, "", msg, &server);
				sendFile(index, &server);
			}
		}
	
	} while (1);

	printf("Client %d disconnected\n", index);
	pthread_exit(0);
}

/**
 * Receives a message from a client.
 * 
 * @param index Index of the client.
 * @param msg Pointer to the buffer to store the received message.
 * @param server Pointer to the ChatServer structure.
 * @return Length of the received message or -1 on failure.
 */
int receiveMessage(int index, char** msg, ChatServer* server) {
	int msgLength;
	if (recv(server->clients[index].chat_socket, &msgLength, sizeof(int), 0) <= 0) {return -1;}
	
	if (server->clients[index].salon == -1) {
		printf("Client %d is connected to salon %d\n", index, msgLength);
		server->clients[index].salon = msgLength;
		return 0;
	}
	
	char* msgRecv = (char*)calloc(msgLength, sizeof(char));
	if (recv(server->clients[index].chat_socket, msgRecv, msgLength*sizeof(char), 0) <= 0) {free(msgRecv); msgRecv = NULL; return -1;}

	if (server->clients[index].username == NULL) {
		printf("Client %d is named %s", index, msgRecv);
		server->clients[index].username = msgRecv;
		return 0;
	}

	*msg = msgRecv;
	return 1;
}

/**
 * Sends a message to a client.
 * 
 * @param sender Index of the sender.
 * @param receiver Index of the receiver.
 * @param username Username to be sent.
 * @param msg Message to be sent.
 * @param server Pointer to the ChatServer structure.
 * @return 0 on success or -1 on failure.
 */
int sendMessage(int receiver, const char* username, const char* msg, ChatServer* server) {
	int usernameLength = strlen(username) + 1;
	if (send(server->clients[receiver].chat_socket, &usernameLength, sizeof(int), 0) < 0) {return -1;}
	if (send(server->clients[receiver].chat_socket, username, usernameLength*sizeof(char), 0) < 0) {return -1;}

	int msgLength = strlen(msg) + 1;
	if (send(server->clients[receiver].chat_socket, &msgLength, sizeof(int), 0) < 0) {return -1;}
	if (send(server->clients[receiver].chat_socket, msg, msgLength*sizeof(char), 0) < 0) {return -1;}
	return 0;
}

/**
 * Broadcasts a message to all clients except the sender.
 * 
 * @param sender_index Index of the client who sent the message.
 * @param msg Message to be broadcasted.
 * @param server Pointer to the ChatServer structure.
 */
void broadcastMessage(int index, const char* msg, ChatServer* server) {
	for (int i=0; i < server->max_clients; i++) {
		if (i != index && server->clients[index].salon == server->clients[i].salon && server->clients[i].chat_socket != -1) {
			sendMessage(i, server->clients[index].username, msg, server);
		}
	}
}