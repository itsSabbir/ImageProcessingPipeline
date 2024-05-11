#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>    /* Internet domain header */

#include "socket.h"
#include "request.h"
#include "response.h"

#ifndef PORT
#define PORT 30000
#endif

#define BACKLOG 10
#define MAX_CLIENTS 10



void handle_get_request(ClientState *client) {
    if (strcmp(client->reqData->path, MAIN_HTML) == 0) {
        main_html_response(client->sock);
    } else if (strcmp(client->reqData->path, IMAGE_FILTER) == 0) {
        image_filter_response(client->sock, client->reqData);
    } else {
        not_found_response(client->sock);
    }
}

void handle_post_request(ClientState *client) {
    image_upload_response(client);
}

/*
 * Read data from a client socket, and, if there is enough information to
 * determine the type of request, spawn a child process to respond to the 
 * request.
 *
 * Return 1 if one of the conditions hold:
 *   a) No bytes were read from the socket. (The client has likely closed the 
 *      connection.)
 *   b) A child process has been created to respond to the request.
 *
 * This return value indicates that the server process should close the socket.
 * Otherwise, return 0 (indicating that the server must continue to monitor the 
 * socket).
 *
 * Complete this function according to the comments within.
 * Note that you'll be doing this incrementally, adding to the function as you
 * complete the different parts of the assignment.
 */
int handle_client(ClientState *client) {
    if (read_from_client(client) < 1) {
        return 1;
    }
    if (parse_req_start_line(client) == 0) {
        return 0;
    }

    pid_t result = fork();
    if (result > 0) {
        return 1; // Parent process
    } else if (result == 0) {
        // Child process
        if (strcmp(client->reqData->method, GET) == 0) {
            handle_get_request(client);
        } else if (strcmp(client->reqData->method, POST) == 0) {
            handle_post_request(client);
        } else {
            not_found_response(client->sock);
        }
        exit(0);
    } else {
        perror("Failed to fork.");
        exit(1);
    }
}




int main(int argc, char **argv) {
    ClientState *clients = init_clients(MAX_CLIENTS);

    struct sockaddr_in *servaddr = init_server_addr(PORT);

    // Create an fd to listen to new connections.
    int listenfd = setup_server_socket(servaddr, BACKLOG);
    
    // Print out information about this server
    char host[MAX_HOSTNAME];
    if ((gethostname(host, sizeof(host))) == -1) {
        perror("gethostname");
        exit(1);
    }
    fprintf(stderr, "Server hostname: %s\n", host);
    fprintf(stderr, "Port: %d\n", PORT);

    // Set up the arguments for select
    int maxfd = listenfd;
    fd_set allset;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    
    // Set up a timer for select (This is only necessary for debugging help)
    struct timeval timer;


    // Main server loop.
    while (1) {
        fd_set rset = allset;
        timer.tv_sec = 2;
        timer.tv_usec = 0;
        int nready = select(maxfd + 1, &rset, NULL, NULL, &timer);
        if(nready == -1) {
            perror("select");
            exit(1);
        }
        
        if(nready == 0) {  // timer expired
            // Check if any children have failed
            int status;
            int pid;
            errno = 0;
            if((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if(WIFSIGNALED(status)) {
                    fprintf(stderr, "Child [%d] failed with signal %d\n", pid,
                            WTERMSIG(status));
                }
            }
            continue;
        }
        

        if (FD_ISSET(listenfd, &rset)) {    // New client connection.
            int new_client_fd = accept_connection(listenfd);
            if (new_client_fd >= 0) {
                maxfd = (new_client_fd > maxfd) ? new_client_fd : maxfd;
                FD_SET(new_client_fd, &allset);    // Add new descriptor to set.

                for(int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sock < 0) {
                        clients[i].sock = new_client_fd;
                        break;
                    }
                }
            }

            nready -= 1;
        }

        // The nready is just an optimization; no harm in checking all fds 
        // except efficiency
        for (int i = 0; i < MAX_CLIENTS && nready > 0; i++) {
            // Check whether clients[i] has an active, ready socket.
            if (clients[i].sock < 0 || !FD_ISSET(clients[i].sock, &rset)) {
                continue;
            }

            int done = handle_client(&clients[i]);
            if (done) {
                FD_CLR(clients[i].sock, &allset);
                remove_client(&clients[i]);
            }

            nready -= 1;
        }
    }
}
