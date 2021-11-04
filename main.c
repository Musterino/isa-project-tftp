#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>
#include <malloc.h>
#include <stdlib.h>

#include "utilities.h"

int main(int argc, char *argv[]) {
    int sock, msg_size, i;
    struct sockaddr_in server, from;
    socklen_t len;
    char buffer[BUFFER];

    // alocate memory for parameters from stdin
    struct prms *parameters = malloc(sizeof(struct prms));
    if (parameters == NULL) {
        err(1, "Unable to alocate memory!");
    }
    parameters->address = malloc(sizeof(BUFFER));
    parameters->d = malloc(sizeof(BUFFER));
    if (parameters->d == NULL || parameters->address == NULL) {
        err(1, "Unable to alocate memory!");
    }

    //create a client socket
    if ((sock = socket(AF_INET , SOCK_DGRAM , 0)) == -1)
        err(1,"socket() failed\n");

    // read input data from STDIN
    //while((msg_size=read(STDIN_FILENO,buffer,BUFFER)) > 0) {
        msg_size = read(STDIN_FILENO, buffer, BUFFER);
        if (parse_parameters(parameters, buffer) != 0) {
            // error occured during parsing
            //continue;
            return 1;
        }
    //}

    printf(
            "Filled parameters:\n"
            "Read|Write:    %d\n"
            "File:          %s\n"
            "Timeout:       %d\n"
            "Size:          %d\n"
            "Multicast      %d\n"
            "Mode:          %d\n"
            "Address:       %s\n"
            "Port:          %d\n",
           parameters->RW, parameters->d, parameters->t, parameters->s, parameters->m, parameters->c, parameters->address, parameters->port);

    // free alocated memory
    free(parameters->address);
    free(parameters->d);
    free(parameters);
    return 0;
}

