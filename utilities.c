//
// Created by musterino on 11/2/21.
//
#include "utilities.h"


int parse_parameters(struct prms *params, char *input) {

    char *ptr;
    char *address_port = NULL;
    int port;

    // set default values
    params->RW = params->t = -1;
    strcpy(params->d, "");
    params->s = 512;
    params->c = "octet";
    params->m = false;
    strcpy(params->address, "127.0.0.1");
    params->port = 69;

    // split input
    char *token = strtok(input, " ");

    // get parameters
    while (token != NULL) {
        if (token[0] != '-') {
            fprintf(stderr, "Invalid parameters!");
            return 1;
        }
        switch (token[1]) {
            case 'R':
                params->RW = 0;
                break;
            case 'W':
                params->RW = 1;
                break;
            case 'd':
                token = strtok(NULL, " ");
                // remove possible next line
                token[strcspn(token, "\n")] = '\0';
                strcpy(params->d, token);
                break;
            case 't':
                ptr = NULL;
                token = strtok(NULL, " ");
                if (token != NULL) {
                    params->t = strtol(token, &ptr, 10);
                    if (strcmp(ptr, "") != 0) {
                        fprintf(stderr, "Invalid timeout time!");
                        return 1;
                    }
                }
                break;
            case 's':
                ptr = NULL;
                token = strtok(NULL, " ");
                if (token != NULL) {
                    long size = strtol(token, &ptr, 10);
                    //TODO: size of packet check if wrong
                    /*if (strcmp(ptr, "") != 0 || size % 8 != 0) {
                        fprintf(stderr, "Invalid block max size!");
                        return 1;
                    }*/
                    params->s = size;
                }
                break;
            case 'm':
                params->m = true;
                break;
            case 'c':
                token = strtok(NULL, " ");
                if (token != NULL) {
                    token[strcspn(token, "\n")] = '\0';
                    strcpy(params->d, token);
                    //TODO: to lowercase for AsCii etc.
                    if (strcmp(params->c, "ascii") == 0 || strcmp(params->c, "netascii") == 0) {
                        params->is_binary = false;
                    } else if (strcmp(params->c, "binary") == 0  || strcmp(params->c, "octet") == 0 ) {
                        params->is_binary = true;
                    } else {
                        fprintf(stderr, "Invalid mode: %s", token);
                        return 1;
                    }
                }
                break;
            case 'a':
                address_port = malloc(BUFFER);
                token = strtok(NULL, " ");
                strcpy(address_port, token);
                break;
            default:
                fprintf(stderr, "Invalid parameter!");
                return 1;
        }
        token = strtok(NULL, " ");
    }
    if (address_port != NULL){
        ptr = NULL;
        address_port = strtok(address_port, ",");
        if (address_port != NULL) {
            //TODO: validate IPv4 and IPv6
            strcpy(params->address, address_port);
        }
        address_port = strtok(NULL, ",");
        if (address_port != NULL) {
            port = strtol(address_port, &ptr, 10);
            ptr[strcspn(ptr, "\n")] = '\0';
            if (strcmp(ptr, "") != 0 || port < 1 || port > 65535) {
                fprintf(stderr, "Invalid port!\n");
                return 1;
            }
            params->port = port;
        } else {
            fprintf(stderr, "Bad address!\n");
            return 1;
        }
        free(address_port);
    }

    if (params->RW == -1 || strcmp(params->d, "") == 0) {
        fprintf(stderr, "Some required parameters are not inputted!");
        return 1;
    }
    return 0;
}