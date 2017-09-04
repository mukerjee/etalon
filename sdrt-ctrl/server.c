#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define NLOOPS 1000000

const char *port = "8888";
int numConnections = 0;

pthread_mutex_t lock;

struct traffic_info {
    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    size_t size;
};

struct workerArgs
{
	int socket;
};

void *accept_clients(void *args);
void *service_single_client(void *args);

int main(int argc, char *argv[])
{
	pthread_t server_thread;

	sigset_t new;
	sigemptyset (&new);
	sigaddset(&new, SIGPIPE);
	if (pthread_sigmask(SIG_BLOCK, &new, NULL) != 0) 
	{
		perror("Unable to mask SIGPIPE");
		exit(-1);
	}

	pthread_mutex_init(&lock, NULL);

	if (pthread_create(&server_thread, NULL, accept_clients, NULL) < 0)
	{
		perror("Could not create server thread");
		exit(-1);
	}

	pthread_join(server_thread, NULL);

	pthread_mutex_destroy(&lock);

	pthread_exit(NULL);
}

void *accept_clients(void *args)
{
	int serverSocket;
	int clientSocket;
	pthread_t worker_thread;
	struct addrinfo hints, *res, *p;
	struct sockaddr_storage *clientAddr;
	socklen_t sinSize = sizeof(struct sockaddr_storage);
	struct workerArgs *wa;
	int yes = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &res) != 0)
	{
		perror("getaddrinfo() failed");
		pthread_exit(NULL);
	}

	for(p = res;p != NULL; p = p->ai_next) 
	{
		if ((serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("Could not open socket");
			continue;
		}

		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("Socket setsockopt() failed");
			close(serverSocket);
			continue;
		}

		if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("Socket bind() failed");
			close(serverSocket);
			continue;
		}

		if (listen(serverSocket, 5) == -1)
		{
			perror("Socket listen() failed");
			close(serverSocket);
			continue;
		}

		break;
	}

	freeaddrinfo(res);

	if (p == NULL)
	{
		fprintf(stderr, "Could not find a socket to bind to.\n");
		pthread_exit(NULL);
	}

	while (1)
	{
		clientAddr = malloc(sinSize);
		if ((clientSocket = accept(serverSocket, (struct sockaddr *) clientAddr, &sinSize)) == -1) 
		{
			free(clientAddr);
			perror("Could not accept() connection");
			continue;
		}

		wa = malloc(sizeof(struct workerArgs));
		wa->socket = clientSocket;

		if (pthread_create(&worker_thread, NULL, service_single_client, wa) != 0) 
		{
			perror("Could not create a worker thread");
			free(clientAddr);
			free(wa);
			close(clientSocket);
			close(serverSocket);
			pthread_exit(NULL);
		}
	}

	pthread_exit(NULL);
}

void *service_single_client(void *args) {
	struct workerArgs *wa;
	int socket, nbytes, i;
	struct traffic_info info;

	wa = (struct workerArgs*) args;
	socket = wa->socket;

	pthread_detach(pthread_self());

	pthread_mutex_lock (&lock);

	for(i=0; i< NLOOPS; i++)
		numConnections++;
	for(i=0; i< NLOOPS-1; i++)
		numConnections--;
	fprintf(stderr, "+ Number of connections is %d\n", numConnections);

	pthread_mutex_unlock (&lock);

	while(1)
	{
		nbytes = read(socket, &info, sizeof(info));
		if (nbytes == 0)
			break;
		else if (nbytes == -1)
		{
			perror("Socket read() failed");
			close(socket);
			pthread_exit(NULL);
		}
        fprintf(stderr, "[CTRL] SRC: %s DST: %s SIZE: %ld\n", info.src, info.dst, info.size);
	}

	pthread_mutex_lock (&lock);
	for(i=0; i< NLOOPS; i++)
		numConnections--;
	for(i=0; i< NLOOPS-1; i++)
		numConnections++;
	fprintf(stderr, "- Number of connections is %d\n", numConnections);
	pthread_mutex_unlock (&lock);

	close(socket);
	pthread_exit(NULL);
}
