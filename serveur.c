#include "srcServeur/headers/chat.h"

ChatServer args;

void sigint_handler(int sig) {
	shutdownServer(&args);
    exit(0);
}

int main(int argc, char *argv[]) {
	// Vérification des paramètres
	if(argc != 5) {
		printf("Erreur: format de commande: ./serveur <NB_CLIENTS> <PORT> <PORT_FILE> <NB_SALONS>\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, sigint_handler);

	initChatServer(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), &args);

	while(1) {
		int index;
		do {
			if ((index = acceptClient(&args)) > -1) {startChatSession(index, &args);}
		} while(index > -1);
	}
}