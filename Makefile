CC = gcc
CFLAGS = -Wall -o
SRCSSERV = srcServeur/*.c
SRCSCLI = srcClient/*.c

all: clean client serveur

client: client.c
	$(CC) $(CFLAGS) $@ $^ $(SRCSCLI)

serveur: serveur.c
	$(CC) $(CFLAGS) $@ $^ $(SRCSSERV)

clean:
	rm -f client serveur