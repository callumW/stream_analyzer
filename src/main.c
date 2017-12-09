#include "libucsi/transport_packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#define DEFAULT_PORT 5000

int stop;
void signal_handler(int signum)
{
    stop = 1;
    printf(" Received stop signal\nStopping....\n");
}

int create_socket(uint16_t port)
{
    int sock;
    struct sockaddr_in name;

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*) &name, sizeof(name)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int close_socket(int sock)
{
    return shutdown(sock, 2);
}

int join_multicast(char** ip, int socket)
{
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    inet_aton(ip, &mreq.imr_multiaddr);
    if (setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("join multicast");
        return -1;
    }
    return socket;
}

int leave_multicast()
{
    return -1;
}

int main(int argc, char** argv)
{
    char* addr = NULL;
    char* port = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
            case 'a':
                addr = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case '?':
                switch (optopt) {
                    case 'a':
                        break;
                    case 'p':
                        break;
                    default:
                        printf("Unknown option -%c\n", optopt);
                }
                return EXIT_FAILURE;
        }
    }

    if (addr == NULL) {

    }
    stop = 0;
    signal(SIGINT, signal_handler);
    printf("Transport packets are %d bytes long!\n", TRANSPORT_PACKET_LENGTH);
    int socket = create_socket(5000);
    printf("Created socket!\n");
    while (!stop) {
        sleep(1);
    }
    close_socket(socket);
    printf("Socket closed\n");
    return 0;
}
