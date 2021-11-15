#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>

#include "utilities.h"
#include "tftpclient.h"

int main() {
    char buffer[BUFFER];
    int parse_ret;

    // alocate memory for parameters from stdin
    struct prms *parameters = malloc(sizeof(struct prms));
    parameters->address = malloc(BUFFER);
    parameters->d = malloc(BUFFER);
    parameters->c = malloc(BUFFER);
    if (parameters == NULL|| parameters->d == NULL || parameters->address == NULL || parameters->c == NULL) {
        err(1, "Unable to alocate memory!");
    }
    printf("Welcome to TFTPv2 client. To send a request enter specified parameters.\n"
           "Note that every parameter must be inputted only once!\n"
           "Parameters: \n"
           "    -R or -W            -   read or write request,  required\n"
           "    -d <path_to_file>   -   specify path to file,   required\n"
           "    -t <secs>           -   timeout in seconds,     optional\n"
           "    -s <num_of_bytes>   -   max block size in \n"
           "                            multiples of octets,    optional\n"
           "    -c <mode>           -   mode of transfer,       optional\n"
           "    -a <address,port>   -   IP address and port\n"
           "                            of TFTP server,         optional\n"
           "To exit the client type q and enter!\n");
    // read input data from STDIN
    while(1) {

        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        parse_ret = parse_parameters(parameters, buffer);
        if (parse_ret != 0) {
            if (parse_ret == 2) {
                break;
            }
            // error occured during parsing
            continue;
        }

        start_tftp_transfer(parameters);

    }


    // free alocated memory
    free(parameters->address);
    free(parameters->d);
    free(parameters);
    return 0;
}

