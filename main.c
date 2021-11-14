#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>

#include "utilities.h"
#include "tftpclient.h"

int main() {
    char buffer[BUFFER];

    // alocate memory for parameters from stdin
    struct prms *parameters = malloc(sizeof(struct prms));
    parameters->address = malloc(BUFFER);
    parameters->d = malloc(BUFFER);
    parameters->c = malloc(BUFFER);
    /*if (parameters || !parameters->d || !parameters->address || parameters->c) {
        err(1, "Unable to alocate memory!");
    }*/

    // read input data from STDIN
    //while(read(STDIN_FILENO,buffer,BUFFER) > 0) {
        read(STDIN_FILENO, buffer, BUFFER);
        if (parse_parameters(parameters, buffer) != 0) {
            // error occured during parsing
            //continue;
            return 1;
        }

        start_tftp_transfer(parameters);

    //}

    /*printf(
            "Filled parameters:\n"
            "Read|Write:    %d\n"
            "File:          %s\n"
            "Timeout:       %d\n"
            "Size:          %d\n"
            "Multicast      %d\n"
            "Mode:          %s\n"
            "Address:       %s\n"
            "Port:          %d\n",
           parameters->RW, parameters->d, parameters->t, parameters->s, parameters->m, parameters->c, parameters->address, parameters->port);
    */



    // free alocated memory
    free(parameters->address);
    free(parameters->d);
    free(parameters);
    return 0;
}

