#include "srcClient/headers/chat.h"

int main(int argc, char *argv[]) {

  // Vérification des paramètres
  if(argc != 4){
    printf("Erreur: format de commande: ./client <ServeurIP> <PORT> <FILEPORT>\n");
    exit(EXIT_FAILURE);
  }

  chat_args args;
  createChat(&args, argv[1], atoi(argv[2]), atoi(argv[3]));

  launchChat(&args);
}