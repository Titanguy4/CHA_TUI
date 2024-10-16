#include "./headers/chat.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int countLines(const char* msg) {
  int msgSize = strlen(msg) / 66;
  int lines = 0;
  while (*msg != '\0') {
      if (*msg == '\n') {
          lines++;
      }
      msg++;
  }
  return (lines > msgSize) ? lines : msgSize;
}

int recvMsgLength(chat_args *args) {
  int messageLength;
  if (recv(args->dS, &messageLength, sizeof(int), 0) <= 0) {return -1;} 
  return messageLength;
}

int recvMsg(chat_args *args, int messageLength, char** messageRecu) {
  char* msg = (char*)malloc(messageLength);
  if (recv(args->dS, msg, messageLength*sizeof(char), 0) <= 0) {
    free(msg);
    return -1;
  }

  *messageRecu = msg;
  return 1;
}

void display(const char* username, const char* msg) {
  int nbLignes = countLines(msg);
  printf("\033[s\033[%dL", nbLignes);
  printf("\t/%s> %s", username, msg);
  printf("\033[u\033[%dB", nbLignes);
  fflush(stdout); // force la maj de la sortie standard
}

/**
 * fonction qui permet d'envoyer : taille message + message
*/
int sendMessage(chat_args *args, char* message) {
  int messageLength = strlen(message)+1;
  // envoie de la taille du message
  if (send(args->dS, &messageLength, sizeof(int), 0) < 0) {
    perror("messageLength send failed\n");
    return -1;
  }
  // envoie du message
  if (send(args->dS, message, messageLength*sizeof(char), 0) < 0) {
    perror("message send failed\n");
    return -1;
  }
  return 1;
}

/**
 * permet de demander le username du client
 * @return{int} si erreur lors de l'envoie du nom d'utilisateur renvoie -1
 *              si aucune erreurs lors de l'envoie renvoie 1
*/
int askUsername(chat_args *args) {
  char* messageEnvoie = (char*)malloc(args->tailleMess);
  printf("\t> Veuillez entrer votre Username :\n");
  printf("\t> ");
  if (fgets(messageEnvoie, args->tailleMess, stdin) == NULL) {
    sendMessage(args, "/bye\n");
  } else {
    if (sendMessage(args, messageEnvoie) < 0) {
      perror("Username binding failed\n");
      return -1;
    }
  }
  return 1;
}

/**
 * permet au client de lire un message recu
 * est utilisé dans un thread
 * @return {void*}
*/
void* reception(void* t){
  chat_args* args = (chat_args*)t;
  
  int msgLength;
  char* username;
  char* msg;
  do {
    if ((msgLength = recvMsgLength(args)) <= 0) {break;}
    if (recvMsg(args, msgLength, &username) < 0) {break;}
    if (msgLength > 1) {username[msgLength-2] = '\0';}
    else {username[0] = '\0';}

    if ((msgLength = recvMsgLength(args)) <= 0) {break;}
    if (recvMsg(args, msgLength, &msg) < 0) {break;}

    int command;
    if ((command = checkCommand(msg)) == 1) {
      pthread_mutex_lock(&mutex);
      recvFile(args);
      pthread_mutex_unlock(&mutex);
    }
    else {
      display(username, msg);
    }

    free(username);
    free(msg);
  } while (1);
  pthread_exit(0);
}

/**
 * permet au client de saisir un message
 * est utilisé dans un thread
 * @return {void*}
*/
void* saisie(void* t){
  chat_args* args = (chat_args*)t;

  do {
    printf("\t> ");
    fflush(stdout);
    char* messageEnvoie = malloc(args->tailleMess);
    if (fgets(messageEnvoie, args->tailleMess, stdin) == NULL) {
      sendMessage(args, "/bye\n");
    } else {
      if (sendMessage(args, messageEnvoie) < 1) {break;}
      int command;
      if ((command = checkCommand(messageEnvoie)) == 0) {sendFile(args);}
      if (command == 1) {
        usleep(100000);
        pthread_mutex_lock(&mutex);
        pthread_mutex_unlock(&mutex);
      }
    }
    free(messageEnvoie);
    
  } while (1);
  pthread_exit(0);
}

void launchChat(chat_args* args) {
  printf("\x1b[34m"); // changement de couleur du texte pour la connexion
  
  int nb_salons;
  recv(args->dS, &nb_salons, sizeof(int), 0);

  int salon;
  printf("\tA quel salon souhaitez-vous vous connecter ? [0-%d] : ", nb_salons-1);
  scanf("%d", &salon);
  while (getchar() != '\n');
  send(args->dS, &salon, sizeof(int), 0);

  // demande du nom d'utilisateur
  if (askUsername(args) > 0) {
    printf("\x1b[32m\n"); // changement de couleur du texte pour la discussion

    pthread_create(&args->tsaisie, NULL, saisie, (void*)args); // Envoie d'un message
    pthread_create(&args->treception, NULL, reception, (void*)args); // Reception d'un message

    pthread_join(args->treception, 0);
  }

  shutdownClient(args);
}