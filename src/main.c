#include <libucsi/transport_packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

#define DEFAULT_PORT 5000

/** GLOBALS **/
char g_ip_addr[16]; // xxx.xxx.xxx.xxx\0 <- 16 chars
uint16_t g_port = 0;
struct ip_mreq g_mreq;
struct sockaddr_in g_name;
bool g_stop = 0;
int g_socket = 0;
uint64_t pkt_count = 0;

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
    // Allow multiple sockets to use same port number
    uint flag = 1;
    if (setsockopt(g_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
        perror("setsockopt");
        return -1;
    }
    if (bind(g_socket, (struct sockaddr*) &g_name, sizeof(g_name)) < 0) {
        perror("bind");
        return -1;
    }
    // set to non blocking
    fcntl(g_socket, F_SETFL, O_NONBLOCK);
    return g_socket;
}

int close_socket(int sock)
{
    return shutdown(sock, 2);
}

bool join_multicast()
{
    if (setsockopt(g_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &g_mreq, sizeof(g_mreq)) < 0) {
        perror("join multicast");
        return false;
    }
    return true;
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
            printf("Using ip address: %s\n", g_ip_addr);
            return true;
        }
    }
    else {
        fprintf(stderr, "%s was passed a null pointer\n", __func__);
        return false;
    }
    return false;
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
                printf("Using port: %"PRIu16"\n", g_port);
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
        g_name.sin_port = htons(g_port);
    }
    return true;
}

void print_packet_info(struct transport_values* pkt_vals)
{
    if ( pkt_vals == NULL) return;

    printf( "Packet No.:     %"PRIu64"\n"
            "Payload Length: %"PRIu16"B\n"
            "PCR:            %"PRIu64"\n"
            "opcr:           %"PRIu64"\n"
            "----------------\n",
            pkt_count,
            pkt_vals->payload_length,
            pkt_vals->pcr,
            pkt_vals->opcr);
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
    printf("Parsing options\n");
    if ( !parse_options(argc, argv) ) return EXIT_FAILURE;
    printf("Initialising\n");
    if ( !init() ) return EXIT_FAILURE;
    printf("Joinging multicast group\n");
    if ( !join_multicast() ) return EXIT_FAILURE;

    struct transport_packet* packet;
    struct transport_values pkt_vals;
    // Extract all
    enum transport_value extract_flag = transport_value_pcr | transport_value_opcr
        | transport_value_splice_countdown | transport_value_private_data | transport_value_ltw |
        transport_value_piecewise_rate | transport_value_seamless_splice;
    size_t to_read = 188 * 7;
    uint8_t buffer[188 * 7] = {0};
    size_t num_read = 0;
    int i = 0;
    printf("Entering main loop\n");
    while (!g_stop) {
        num_read = read(g_socket, buffer, to_read);
        if ( (unsigned) num_read == -1 ) {
            fprintf(stderr, "Failed to read anything\n");
            perror("socket read");
        }
        if ( num_read % TRANSPORT_PACKET_LENGTH == 0) {
            for (i = 0; i < num_read; i += TRANSPORT_PACKET_LENGTH) {
                packet = transport_packet_init( (unsigned char*) buffer + i);
                transport_packet_values_extract(packet, &pkt_vals, extract_flag);
                // print packet info
                print_packet_info(&pkt_vals);
                pkt_count++;
            }
        }
        else {
            fprintf(stderr, "Read in an unaligned amount of data (%ldB)\n", num_read);
        }
        // get packets from udp frame
        // calculate stats
        // check if we need to print stats
        // sleep
        sleep(1);
    }
    close_socket(g_socket);
    printf("Socket closed\n");
    return EXIT_SUCCESS;
}
