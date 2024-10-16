#include "headers/chat.h"

#define SEM_KEY "key.txt"

/**
 * Initializes the chat server, sets up the server sockets for chat and file transfer, and listens for incoming connections.
 * 
 * @param max_clients Maximum number of clients the server can handle.
 * @param chat_port Port number on which the server will listen for chat connections.
 * @param file_port Port number on which the server will listen for file transfer connections.
 * @param server Pointer to the ChatServer structure.
 * @return 0 if done or -1 on failure.
 */
int initChatServer(int max_clients, int chat_port, int file_port, int nb_salons, ChatServer* server) {
    struct sockaddr_in chat_addr, file_addr;

    // Initialize server structure
    server->max_clients = max_clients;
    server->nb_salons = nb_salons;
    server->clients = (User*)malloc(sizeof(User) * max_clients);
    if (!server->clients) {
        perror("Failed to allocate memory for clients");
        return -1;
    }
	for (int i=0; i<max_clients; i++) {
		server->clients[i].chat_socket = -1;
        server->clients[i].salon = -1;
	}
    pthread_mutex_init(&server->lock, NULL);

    // Create chat socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        perror("Chat socket creation failed");
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

    // Create file transfer socket
    server->file_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->file_server_socket < 0) {
        perror("File socket creation failed");
        close(server->server_socket);
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

	int optval = 1;

    // Prepare chat address
    chat_addr.sin_family = AF_INET;
    chat_addr.sin_addr.s_addr = INADDR_ANY;
    chat_addr.sin_port = htons(chat_port);
    setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // Permet de réutiliser un socket

    // Prepare file transfer address
    file_addr.sin_family = AF_INET;
    file_addr.sin_addr.s_addr = INADDR_ANY;
    file_addr.sin_port = htons(file_port);
    setsockopt(server->file_server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // Permet de réutiliser un socket

    // Bind chat socket
    if (bind(server->server_socket, (struct sockaddr*)&chat_addr, sizeof(chat_addr)) < 0) {
        perror("Chat socket bind failed");
        close(server->server_socket);
        close(server->file_server_socket);
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

    // Bind file transfer socket
    if (bind(server->file_server_socket, (struct sockaddr*)&file_addr, sizeof(file_addr)) < 0) {
        perror("File socket bind failed");
        close(server->server_socket);
        close(server->file_server_socket);
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

    // Listen on chat socket
    if (listen(server->server_socket, max_clients) < 0) {
        perror("Chat socket listen failed");
        close(server->server_socket);
        close(server->file_server_socket);
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

    // Listen on file transfer socket
    if (listen(server->file_server_socket, max_clients) < 0) {
        perror("File socket listen failed");
        close(server->server_socket);
        close(server->file_server_socket);
        free(server->clients);
        server->clients = NULL;
        return -1;
    }

    // Init of sems
    int cle = ftok(SEM_KEY, 'r');
    if (cle == -1) {
        perror("ftok error");
        return -1;
    }

    int idSem = semget(cle, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (idSem == -1) {
        perror("semget error");
        return -1;
    }

    ushort tabinit[1] = { max_clients };

    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } valinit;

    valinit.array = tabinit;

    if (semctl(idSem, 0, SETALL, valinit) == -1) {
        perror("semctl initialization error");
        return -1;
    }

    printf("Chat created\n");

    return 0;
}

int emptyClient(ChatServer* server) {
	for (int i=0; i<server->max_clients; i++) {
		if (server->clients[i].chat_socket == -1) {
			return i;
		}
	}
	return -1;
}

/**
 * Accepts a new client connection for chat, and adds the client to the server.
 * 
 * @param server Pointer to the ChatServer structure.
 * @return Index of the newly connected client or -1 on failure.
 */
int acceptClient(ChatServer* server) {
	pthread_mutex_lock(&server->lock);
	
    int cle = ftok(SEM_KEY, 'r');
    if (cle == -1) {
        perror("ftok error");
        return -1;
    }

	int idSem = semget(cle, 1, 0600);
    if (idSem == -1) {
        perror("semget error");
        return -1;
    }

	struct sembuf op[]={
		{(ushort)0,(short)-1,0},
		{(ushort)0,(short)+1,0},
		{(ushort)0, (short)0,0}};

	semop(idSem,op,1);

	pthread_mutex_unlock(&server->lock);

	int index = emptyClient(server); // recupere l'index du nouveau client dans le tableau de clients

	if (index > -1) {
		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int new_socket;

		// Accept new chat connection
		new_socket = accept(server->server_socket, (struct sockaddr*)&client_addr, &addr_len);
		if (new_socket < 0) {
		    perror("Accepting new chat client failed");
		    return -1;
		}

		pthread_mutex_lock(&server->lock);
		server->clients[index].chat_socket = new_socket;
		pthread_mutex_unlock(&server->lock);

		printf("Client %d connected\n", index);
		int startCode = 1;
		send(server->clients[index].chat_socket, &startCode, sizeof(int), 0);
        send(server->clients[index].chat_socket, &server->nb_salons, sizeof(int), 0);
		return index;
	}
	return -1;
}

/**
 * Shuts down a specific client connection and removes the client from the server.
 * 
 * @param index Index of the client to be removed.
 * @param server Pointer to the ChatServer structure.
 * @return 0 if done or -1 on failure.
 */
int removeClient(int index, ChatServer* server) {
	int shutdownCode = -1;
	send(server->clients[index].chat_socket, &shutdownCode, sizeof(int), 0);
    pthread_mutex_lock(&server->lock);
    if (index < 0 || index >= server->max_clients) {
        pthread_mutex_unlock(&server->lock);
        fprintf(stderr, "Invalid client index\n");
        return -1;
    }

    // Close client socket and cleanup
    shutdown(server->clients[index].chat_socket,2);
    free(server->clients[index].username);
    server->clients[index].username = NULL;
    server->clients[index].chat_socket = -1;

    int cle = ftok(SEM_KEY, 'r');
    if (cle == -1) {
        perror("ftok error");
        return -1;
    }

	int idSem = semget(cle, 1, 0600);
    if (idSem == -1) {
        perror("semget error");
        return -1;
    }

    pthread_mutex_unlock(&server->lock);

	struct sembuf op[]={
		{(ushort)0,(short)-1,0},
		{(ushort)0,(short)+1,0},
		{(ushort)0, (short)0,0}};

	semop(idSem,op+1,1);
    return 0;
}

/**
 * Shuts down the server, closes all client connections, and cleans up resources.
 * 
 * @param server Pointer to the ChatServer structure.
 * @return 0 if done or -1 on failure.
 */
int shutdownServer(ChatServer* server) {
    // Close all client connections
    for (int i = 0; i < server->max_clients; i++) {
		if (server->clients[i].chat_socket != -1) {
            removeClient(i, server);
		}
    }

    pthread_mutex_lock(&server->lock);
    // Close server sockets
    shutdown(server->server_socket,2);
    shutdown(server->file_server_socket,2);

    // Cleanup resources
    free(server->clients);
    server->clients = NULL;
    
    int cle = ftok(SEM_KEY, 'r');
    if (cle == -1) {
        perror("ftok error");
        return -1;
    }

	int idSem = semget(cle, 1, 0600);
    if (idSem == -1) {
        perror("semget error");
        return -1;
    }
    
    pthread_mutex_unlock(&server->lock);
    pthread_mutex_destroy(&server->lock);

	semctl(idSem, 0, IPC_RMID, NULL);

    return 0;
}