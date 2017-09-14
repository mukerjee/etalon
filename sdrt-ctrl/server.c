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

const char *port = "8123";

struct traffic_info {
    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    size_t size;
};


int main(int argc, char *argv[])
{
    fd_set active_fd_set, read_fd_set;
    int serverSocket;
    int clientSocket;
    int i;
    int nbytes;
    struct traffic_info info;

    struct addrinfo hints, *res, *p;
    struct sockaddr_storage *clientAddr;
    socklen_t sinSize = sizeof(struct sockaddr_storage);
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

	FD_ZERO (&active_fd_set);
	FD_SET (serverSocket, &active_fd_set);

    while (1)
    {
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror ("select");
            exit (EXIT_FAILURE);
        }

        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == serverSocket) {
                    clientAddr = malloc(sinSize);
                    if ((clientSocket = accept(serverSocket, (struct sockaddr *) clientAddr, &sinSize)) == -1) 
                    {
                        free(clientAddr);
                        perror("Could not accept() connection");
                        exit (EXIT_FAILURE);
                    }
                    FD_SET (clientSocket, &active_fd_set);
                    fprintf(stderr, "New connection: %d\n", clientSocket);
                }
                else
                {
                    nbytes = read(i, &info, sizeof(info));
                    if (nbytes == 0)
                    {
                        fprintf(stderr, "Closing socket\n");
                        close(i);
                        FD_CLR(i, &active_fd_set);
                        break;
                    }
                    if (nbytes < 0)
                    {
                        perror("Socket read() failed");
                        close(i);
                        FD_CLR (i, &active_fd_set);
                        exit (EXIT_FAILURE);
                    }
                    fprintf(stderr, "[CTRL] SRC: %s DST: %s SIZE: %ld\n", info.src, info.dst, info.size);
                }
            }
        }
    }
}
