#include "libucsi/transport_packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_PORT 5000

/** GLOBALS **/
char g_ip_addr[16]; // xxx.xxx.xxx.xxx\0 <- 16 chars
uint16_t g_port = 0;
struct ip_mreq g_mreq;
struct sockaddr_in g_name;
bool g_stop = 0;
int g_socket = 0;

void signal_handler(int signum)
{
    g_stop = 1;
    printf(" Received stop signal\nStopping....\n");
}

int create_socket()
{
    g_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (g_socket < 0) {
        perror("socket");
        return -1;
    }
    if (bind(g_socket, (struct sockaddr*) &g_name, sizeof(g_name)) < 0) {
        perror("bind");
        return -1;
    }
    return g_socket;
}

int close_socket(int sock)
{
    return shutdown(sock, 2);
}

int join_multicast(char** ip, int socket)
{
    if (setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &g_mreq, sizeof(g_mreq)) < 0) {
        perror("join multicast");
        return -1;
    }
    return socket;
}

int leave_multicast()
{
    return -1;
}

bool set_ip(char* ip)
{
    if ( ip != NULL) {
        if ( inet_aton(ip, &g_mreq.imr_multiaddr) != 0 ){
            strncpy(g_ip_addr, ip, 15); // copy address, leave last element as null terminator
            return true;
        }
    }
    else {
        fprintf(stderr, "%s was passed a null pointer\n", __func__);
        return false;
    }
}

bool parse_options(int argc, char** argv)
{
    if ( argv == NULL ) return -1;
    bool got_ip = false;
    bool got_port = false;
    int tmp;
    int opt;
    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
            case 'a':   // ip address
                if ( !set_ip(optarg) ) {
                    fprintf(stderr, "%s is not a valid ip address\n", optarg);
                    return false;
                }
                got_ip = true;
                break;
            case 'p':   // port number
                tmp = atoi(optarg);
                if ( tmp >= 0 && tmp <= 65535 )
                    g_port = (uint16_t) tmp;
                else {
                    fprintf(stderr, "%s is not a valid port, using %d instead\n", optarg, DEFAULT_PORT);
                    g_port = DEFAULT_PORT;
                }
                g_name.sin_port = htons(g_port);
                got_port = true;
                break;
            case '?':   // unknown
                switch (optopt) {
                    case 'a':
                        break;
                    case 'p':
                        break;
                    default:
                        fprintf(stderr, "Unknown option -%c\n", optopt);
                }
                return false;
        }
    }
    if ( !got_ip ) {
        fprintf(stderr, "No ip address specified, exiting.\n");
        return false;
    }
    if ( !got_port ) {
        fprintf(stderr, "No port specified, using port %d\n", DEFAULT_PORT);
        g_port = DEFAULT_PORT;
    }
    return true;
}

bool init()  // Must be called after globals are set up
{
    g_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    g_name.sin_addr.s_addr = htonl(INADDR_ANY);
    g_name.sin_family = AF_INET;
    if ( create_socket() < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return false;
    }
    signal(SIGINT, signal_handler);
    return true;
}

int main(int argc, char** argv)
{
    if ( !parse_options(argc, argv) ) return EXIT_FAILURE;
    if ( !init() ) return EXIT_FAILURE;

    while (!g_stop) {
        sleep(1);
    }
    close_socket(g_socket);
    printf("Socket closed\n");
    return EXIT_SUCCESS;
}
