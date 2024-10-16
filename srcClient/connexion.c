#include "./headers/chat.h"

/**
 * permet la création du socket
 * @params {char*} correspond à l'address du serveur
 * @return {int dS} retourne le descripteur du socket qui vient d'etre créé
*/
void createChat(chat_args* args, char* address, int port, int portFile) {
  chat_args res;
  res.dS = socket(PF_INET, SOCK_STREAM, 0); // Création du socket pour le protocole TCP
  res.tailleMess = 256;
  res.portFileServer = portFile;
  res.address = address;

  struct sockaddr_in aS;
  aS.sin_family = AF_INET; // L'IP du serveur sera une IPv4
  inet_pton(AF_INET,address,&(aS.sin_addr)) ; // Permet de spécifier l'adresse du serveur sous forme binaire
  aS.sin_port = htons(port) ; // Permet de spécifier le port sûr lequel se connecter sous forme binaire
  socklen_t lgA = sizeof(struct sockaddr_in);

  if (connect(res.dS, (struct sockaddr *) &aS, lgA) < 0) { // test de la connexion au serveur
    perror("Connection failed");
    exit(EXIT_FAILURE);
  }

  char command[] = "clear";
  system(command);
  printf("Bienvenue, vous serez mis en attente si le serveur est complet.\n");
  int startCode;
  if (recv(res.dS, &startCode, sizeof(int), 0) < 0) {
    perror("Server could not connect you");
    exit(EXIT_FAILURE);
  }

  *args = res;
}

void shutdownClient(chat_args* args) {
  chat_args res = *args;
  shutdown(res.dS,2);
  res.dS = -1;
  printf("\x1b[34m");
  printf("Fin du chat\n");
  printf("\x1b[0m");

  *args = res;
}