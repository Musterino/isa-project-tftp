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
    params->tsize = 0;

    // split input
    char *token = strtok(input, " ");

    // get parameters
    while (token != NULL) {
        if (token[0] == 'q') {
            timestamp();
            printf("Ending client!\n");
            return 2;
        }
        if (token[0] != '-') {
            fprintf(stderr, "Invalid parameters!\n");
            return 1;
        }
        token[strcspn(token, "\n")] = '\0';
        if (strlen(token) == 2) {
            switch (token[1]) {
                case 'R':
                    if (params->RW != -1) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    params->RW = 0;
                    break;
                case 'W':
                    if (params->RW != -1) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    params->RW = 1;
                    break;
                case 'd':
                    if (strcmp(params->d, "") != 0) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    token = strtok(NULL, " ");
                    // remove possible next line
                    token[strcspn(token, "\n")] = '\0';
                    strcpy(params->d, token);
                    break;
                case 't':
                    if (params->t != -1) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    ptr = NULL;
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        token[strcspn(token, "\n")] = '\0';
                        params->t = strtol(token, &ptr, 10);
                        if (strcmp(ptr, "") != 0 || (params->t < 1 && params->t > 255)) {
                            fprintf(stderr, "Invalid timeout time!");
                            return 1;
                        }
                    }
                    break;
                case 's':
                    if (params->t != 512) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    ptr = NULL;
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        token[strcspn(token, "\n")] = '\0';
                        long size = strtol(token, &ptr, 10);
                        if (strcmp(ptr, "") != 0 || size % 8 != 0) {
                            fprintf(stderr, "Invalid block max size!");
                            return 1;
                        }
                        params->s = size;
                    }
                    break;
                case 'm':
                    params->m = true;
                    fprintf(stderr, "Multicast option is not implemented, ignoring!\n");
                    break;
                case 'c':
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        token[strcspn(token, "\n")] = '\0';
                        //TODO: to lowercase for AsCii etc.
                        if (strcmp(token, "ascii") == 0 || strcmp(token, "netascii") == 0) {
                            params->is_binary = false;
                            fprintf(stderr, "ASCII transfer is not implemented!\n");
                            return 1;
                        } else if (strcmp(params->c, "binary") == 0 || strcmp(params->c, "octet") == 0) {
                            params->is_binary = true;
                        } else {
                            fprintf(stderr, "Invalid mode: %s", token);
                            return 1;
                        }
                    }
                    break;
                case 'a':
                    if (address_port != NULL) {
                        fprintf(stderr, "Invalid parameters!\n");
                        return 1;
                    }
                    address_port = malloc(BUFFER);
                    token = strtok(NULL, " ");
                    strcpy(address_port, token);
                    break;
                default:
                    fprintf(stderr, "Invalid parameter!");
                    return 1;
            }
        } else {
            fprintf(stderr, "Invalid parameter: %s\n", token);
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
    }

    if (params->RW == -1 || strcmp(params->d, "") == 0) {
        fprintf(stderr, "Some required parameters are not inputted!");
        return 1;
    }
    return 0;
}

void timestamp() {
    time_t ltime;
    struct tm *timeinfo;
    ltime=time(NULL);
    timeinfo = localtime(&ltime);
    printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}