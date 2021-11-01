#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>
#include <malloc.h>
#include <stdlib.h>


#define BUFFER 1024              // buffer length

struct addr_port {
    char address[BUFFER];
    int port;
};

struct prms {
    int RW;         // read - 0 | write - 1
    char d[BUFFER]; // filepath
    int t;         // timeout (s)
    int s;         // blocksize
    bool m;         // multicast
    bool c;         // mode - ascii - false | binary - true
    struct addr_port *a;     // interface ip address
};

int parse_parameters(struct prms *params, char *input);

int main(int argc, char *argv[]) {
    int sock, msg_size, i;
    struct sockaddr_in server, from;
    socklen_t len;
    char buffer[BUFFER];

    // alocate memory for parameters from stdin
    struct addr_port *addr_port = malloc(sizeof(struct addr_port));
    struct prms *parameters = malloc(sizeof(struct prms));
    if (addr_port == NULL || parameters == NULL) {
        err(1, "Unable to alocate memory!");
    }
    parameters->a = addr_port;

    //create a client socket
    if ((sock = socket(AF_INET , SOCK_DGRAM , 0)) == -1)
        err(1,"socket() failed\n");

    // read input data from STDIN
    while((msg_size=read(STDIN_FILENO,buffer,BUFFER)) > 0) {
        if (parse_parameters(parameters, buffer) != 0) {
            // error occured during parsing
            continue;
        }
    }

    printf(
            "Filled parameters:\n"
            "Read|Write:    %d\n"
            "File:          %s\n"
            "Timeout:       %ld\n"
            "Size:          %ld\n"
            "Multicast      %d\n"
            "Mode:          %d\n"
            "Address:       %s\n"
            "Port:          %ld\n",
           parameters->RW, parameters->d, parameters->t, parameters->s, parameters->m, parameters->c, parameters->a->address, parameters->a->port);

    // free alocated memory
    free(addr_port);
    free(parameters);
    return 0;
}


int parse_parameters(struct prms *params, char *input) {
    // set default values
    params->RW = params->t = -1;
    strcpy(params->d, "");
    params->s = 512;
    params->c = true;
    params->m = false;
    strcpy(params->a->address, "127:0:0:1");
    params->a->port = 69;

    // split input
    char *token = strtok(input, " ");

    // get parameters
    while (token != NULL) {
        char *ptr;
        if (token[0] != '-') {
            printf("Invalid parameters!");
            return 1;
        }
        switch (token[1]) {
            //TODO: DO NOT EXIT WITH ERROR, CONTINUE INSTEAD
            case 'R':
                params->RW = 0;
                break;
            case 'W':
                params->RW = 1;
                break;
            case 'd':
                token = strtok(NULL, " ");
                char *path = strtok(token, "\n");
                if (token != NULL) {
                    strcpy(params->d, path);
                }
                break;
            case 't':
                token = strtok(NULL, " ");
                if (token != NULL) {
                    params->t = strtol(token, &ptr, 10);
                    if (!strcmp(ptr, "")) {
                        printf("Invalid timeout time!");
                        return 1;
                    }
                }
                break;
            case 's':
                token = strtok(NULL, " ");
                if (token != NULL) {
                    long size = strtol(token, &ptr, 10);
                    if (ptr != NULL || size % 8 != 0) {
                        printf("Invalid block max size!");
                        return 1;
                    }
                    params->s = size;
                }
                break;
            case 'm':
                params->m = true;
                break;
            case 'c':
                token = strtok(NULL, " ");
                if (token != NULL) {
                    //TODO: to lowercase for AsCii etc.
                    if (strcmp(token, "ascii") == 0 || strcmp(token, "netascii") == 0) {
                        params->c = false;
                    } else if (strcmp(token, "binary") == 0  || strcmp(token, "octet") == 0 ) {
                        params->c = true;
                    } else {
                        printf("Invalid mode!");
                        return 1;
                    }
                }
                break;
            case 'a':
                //TODO: validate IPv4 and IPv6
                break;
            default:
                printf("Invalid parameter!");
                return 1;
        }
        token = strtok(NULL, " ");
    }
    return 0;
}