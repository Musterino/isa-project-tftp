//
// Created by musterino on 11/6/21.
//

/*TODO:
 * IPv6
 * read with ascii sends request with filename ascii
 * when writing check acknowledgement block number
 * multicast
 * timeout
 * size
 * bad parameter -> segmentation fault
 * Formatted feedback
 *
 */



#include <unistd.h>
#include <errno.h>
#include "tftpclient.h"

int start_tftp_transfer(struct prms *parameters) {
    int sock, msg_size, header_len, ack_ret;
    uint16_t block_num;
    char tftp_header[parameters->s + 16], opcode[2], s_port[5], s_tsize[BUFFER];
    socklen_t len = sizeof(struct sockaddr);
    FILE *filep;        // file pointer
    int total_size;     // total size of file
    bool is_options = false;
    struct addrinfo hints, *addrinfo_server;
    struct sockaddr server;

    memset(&hints, 0, sizeof(hints));
    memset(&server, 0, sizeof(server));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    sprintf(s_port, "%d", parameters->port);

    // get address info - ipv4 or ipv6
    if (getaddrinfo(parameters->address, s_port, &hints, &addrinfo_server) != 0) {
        fprintf(stderr, "Unable to get address.\n");
        return ERROR;
    }

    // create UDP socket
    if ((sock = socket(addrinfo_server->ai_family, addrinfo_server->ai_socktype, addrinfo_server->ai_protocol)) == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        return ERROR;
    }

    // create tftp header
    // set opcode
    if (parameters->RW) {
        *(uint16_t *) opcode = htons(OP_CODE_WRQ);  // set 2 byte WRQ opcode to add to packet
        // get size of file
        fseek(filep, 0L, SEEK_END);
        total_size = parameters->tsize = ftell(filep);
        rewind(filep);
        timestamp();
        printf("Sending WRITE request to server at %s:%d\n", parameters->address, parameters->port);
    } else {
        *(uint16_t *) opcode = htons(OP_CODE_RRQ);  // set 2 byte RRQ opcode to add to packet
        timestamp();
        printf("Sending READ request to server at %s:%d\n", parameters->address, parameters->port);

    }
    // tsize to string
    sprintf(s_tsize, "%d", parameters->tsize);
    // create tftp header
    header_len = sprintf(tftp_header, "%c%c%s%c%s%c%s%c%s%c", opcode[0], opcode[1], parameters->d, 0, parameters->c, 0, "tsize", 0, s_tsize, 0);
    // set block size if requested
    if (parameters->s != 512) {
        char s_blocksize[5];
        sprintf(s_blocksize, "%d", parameters->s);
        // set blocksize
        header_len += sprintf(tftp_header + header_len, "%s%c%s%c", "blksize", 0, s_blocksize, 0);
    }
    // set timeout if requested
    if (parameters->t > 0) {
        char s_timeout[3];
        sprintf(s_timeout, "%d", parameters->t);
        header_len += sprintf(tftp_header + header_len, "%s%c%s%c", "timeout", 0, s_timeout, 0);
    }

    // send request to server
    msg_size = sendto(sock, tftp_header, header_len, 0, addrinfo_server->ai_addr, addrinfo_server->ai_addrlen);



    // get ack packet
    int try;
    for (try = 0; try < 4; ++try) {
        msg_size = recvfrom(sock, tftp_header, parameters->s + 16, 0, &server, &len);
        ack_ret = check_tftp_ack(0, tftp_header, parameters, server);
        if (ack_ret == IS_OPTIONS) {
            is_options = true;
            break;
        } else if (ack_ret == OK) {
            break;
        } else if (ack_ret == ERROR) {
            return ERROR;
        }
    }
    if (try == 4) {
        fprintf(stderr, "Error accepting packet!\n");
        return ERROR;
    }

    // start file transfer
    if (parameters->RW) {
                        //================= WRITE DATA =====================//
        int sent_size = 0;
        char *data = malloc(parameters->s + 1);

        // open file
        if (parameters->is_binary) {
            // open file read binary
            filep = fopen(parameters->d, "rb");
        } else {
            // open file read ascii
            filep = fopen(parameters->d, "r");
        }
        if (filep == NULL) {
            fprintf(stderr, "Could not open file!");
            return 1;
        }

        block_num = 1;

        // start reading and sending file data
        while (sent_size < total_size) {
            // read file data
            if (parameters->s + sent_size < total_size) {
                // remaining size > set size so fill the whole packet
                fread(data, parameters->s, sizeof(char), filep);
                // add end of string
                data[parameters->s] = '\0';
                // add size of data read to sent size
                sent_size += parameters->s;
            } else {
                // remaining size < set size -> last packet to be sent
                fread(data, total_size - sent_size, sizeof(char), filep);
                // add end of string
                data[total_size - sent_size] = '\0';
                sent_size += (total_size - sent_size);
            }

            timestamp();
            printf("Sending data: %d out of %d\n", sent_size, total_size);
            // send data packet
            if (send_tftp_data(sock, server, block_num, data) <= 0) {
                perror("Failed sending data: ");
                return 1;
            }

            // get acknowledgement packet
            for (try = 0; try < 4; ++try) {
                msg_size = recvfrom(sock, tftp_header, parameters->s + 16, 0, &server, &len);
                if (msg_size < 0) {
                    fprintf(stderr, "Failed recvfrom!\n");
                    return 1;
                } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ACK) {
                    if (get_block_num(tftp_header) == block_num)
                        break;
                }
            }
            if (try == 4) {
                fprintf(stderr, "Error accepting ack packet!\n");
                return ERROR;
            }

            block_num++;
        }
        timestamp();
        printf("Transfer was succesful, %d bytes written!\n", total_size);

    } else {
        //================= READ DATA =====================//
        int read_data = 0;  // number of bytes read
        if (is_options) {
            if (send_tftp_ack(sock, server, 0) < 0) {
                perror("Error sending ack packet");
                return 1;
            }
            for (try = 0; try < 4; ++try) {
                msg_size = recvfrom(sock, tftp_header, parameters->s + 16, 0, (struct sockaddr *)&server, &len);
                ack_ret = check_tftp_ack(0, tftp_header, parameters, server);
                if (ack_ret == IS_OPTIONS) {
                    continue;
                } else if (ack_ret == OK) {
                    break;
                }
            }
            if (ack_ret == ERROR || try == 4) {
                fprintf(stderr, "Error accepting packet!\n");
                return ERROR;
            }
        }
        if (parameters->is_binary) {
            // open file write binary
            filep = fopen(parameters->d, "wb");
        } else {
            // open file write ascii
            filep = fopen(parameters->d, "wb");
        }
        if (filep == NULL) {
            fprintf(stderr, "Could not open file!");
            return 1;
        }
        timestamp();
        printf("Reading %d bytes of data from server!\n", parameters->tsize);
        int last_block_number = 0;
        bool is_last = false;
        while (1) {
            if ((msg_size < (parameters->s + 4)) && !is_last) {
                is_last = true;
            }
            if (msg_size == (parameters->s + 4) || is_last) {
                block_num = get_block_num(tftp_header);
                if (ntohs(*(uint16_t *) tftp_header) == OP_CODE_DATA) {
                    // check if data packet isn't duplicate
                    if (last_block_number != block_num) {
                        read_data += msg_size-4;
                        // set last block number to be current
                        last_block_number = block_num;
                        // write data to file, start after opcode and block number - 4 bytes
                        fwrite(tftp_header+4, sizeof(char), msg_size-4, filep);

                        timestamp();
                        printf("Read %d B of %d B total.\n", read_data, parameters->tsize);
                    }
                    // change opcode to acknowledgement
                    *(uint16_t *) tftp_header = htons(OP_CODE_ACK);
                    // send ack packet
                    sendto(sock, tftp_header, 4, 0, (struct sockaddr *) &server, len);
                } else {
                    fprintf(stderr, "Bad header: %d\n", *(uint16_t *) tftp_header);
                    return ERROR;
                }
                // if last block is acknowledged break out of loop
                if (is_last)
                    break;
                // get next data packet
                for (try = 0; try < 4; ++try) {
                    msg_size = recvfrom(sock, tftp_header, parameters->s + 16, 0, (struct sockaddr *)&server, &len);
                    ack_ret = check_tftp_ack(0, tftp_header, parameters, server);
                    if (ack_ret == IS_OPTIONS) {
                        continue;
                    } else if (ack_ret == OK) {
                        break;
                    }
                }
                if (ack_ret == ERROR || try == 4) {
                    fprintf(stderr, "Error accepting packet!\n");
                    return ERROR;
                }
            } else {
                break;
            }
        }
        timestamp();
        printf("Transfer was succesful, %ld bytes read!\n", ftell(filep));
    }
    // close socket
    close(sock);
    return 0;
}


int send_tftp_data(int sock, struct sockaddr server, int block_num, char *data) {
    int header_len;
    char tftp_header[strlen(data) + 16], byte_block_num[2], opcode[2];

    *(uint16_t *) opcode = htons(OP_CODE_DATA);
    *(uint16_t *) byte_block_num = htons(block_num);

    // create tftp data header
    header_len = sprintf(tftp_header, "%c%c%c%c%s", opcode[0], opcode[1], byte_block_num[0], byte_block_num[1], data);

    // send data packet
    return sendto(sock, tftp_header, header_len, 0, &server, sizeof(struct sockaddr));
}

int send_tftp_ack(int sock, struct sockaddr server, int block_num) {
    char tftp_header[5], byte_block_num[2], opcode[2];
    int header_len;

    *(uint16_t *) opcode = htons(OP_CODE_ACK);
    *(uint16_t *) byte_block_num = htons(block_num);

    // create tftp data header
    header_len = sprintf(tftp_header, "%c%c%c%c", opcode[0], opcode[1], byte_block_num[0], byte_block_num[1]);

    return sendto(sock, tftp_header, header_len, 0, &server, sizeof(struct sockaddr));
}

int check_tftp_ack(int block_num, char *tftp_header, struct prms *parameters, struct sockaddr server) {
    struct sockaddr_in *server4;
    struct sockaddr_in6 *server6;
    for (int try = 0; try < 4; ++try) {
        if (strlen(tftp_header) < 0) {
            perror("Failed recvfrom: ");
            continue;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_DATA){
            return OK;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ACK){
            // if options were requested but ack received instead of oack reset blksize
            if (block_num == 0) {
                parameters->s = 512;
            }
            return OK;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_OACK){
            int oack_options = check_oack_options(parameters, tftp_header);
            return IS_OPTIONS;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ERROR) {
            fprintf(stderr, "Received error message: %s\n", tftp_header+4);
            return ERROR;
        } else if (server.sa_family == AF_INET) {
            server4 = (struct sockaddr_in *) &server;
            if (ntohs(server4->sin_port) == 69)
                printf("Not received ack!\n");
            continue;
        } else if (server.sa_family == AF_INET6) {
            server6 = (struct sockaddr_in6 *) &server;
            if (ntohs(server6->sin6_port) == 69)
                printf("Not received ack!\n");
            continue;
        }
    }
    return ERROR;
}

uint16_t get_block_num(char *tftp_header) {
    char byte_block_num[2];
    // get block number
    byte_block_num[0] = tftp_header[2];
    byte_block_num[1] = tftp_header[3];
    return ntohs(*(uint16_t *)byte_block_num);
}

int check_oack_options(struct prms *parameters, char *tftp_oack_header) {
    int oack_index = 2, size, timeout;     // skip opcode (2 bytes)
    char *option;
    int return_value = 0;
    bool is_timeout = false, is_blksize = false;

    option = get_value_from_tftp_header_at_index(tftp_oack_header, oack_index);
    while (option != NULL) {
        oack_index += strlen(option) + 1;
        if (strcmp(option, "blksize") == 0) {
            is_blksize = true;
            free(option);
            option = get_value_from_tftp_header_at_index(tftp_oack_header, oack_index);
            oack_index += strlen(option) + 1;
            size = strtol(option, NULL, 10);
            if (parameters->s != size) {
                timestamp();
                printf("Server acknowledged block size %d, using this instead of %d.\n", size, parameters->s);
                parameters->s = size;
            } else {
                timestamp();
                printf("Server acknowledged block size %d.\n", size);
            }
            free(option);
        } else if (strcmp(option, "tsize") == 0) {
            free(option);
            option = get_value_from_tftp_header_at_index(tftp_oack_header, oack_index);
            oack_index += strlen(option) + 1;
            if (parameters->tsize == 0) {
                parameters->tsize = strtol(option, NULL, 10);
            }
            free(option);
        } else if (strcmp(option, "timeout") == 0) {
            is_timeout = true;
            free(option);
            option = get_value_from_tftp_header_at_index(tftp_oack_header, oack_index);
            oack_index += strlen(option) + 1;
            timeout = strtol(option, NULL, 10);
            if (parameters->t != timeout) {
                timestamp();
                printf("Server acknowledged timeout %d, using this instead of %d.\n", timeout, parameters->s);
                parameters->t = timeout;
            } else {
                timestamp();
                printf("Server acknowledged timeout %d.\n", timeout);
            }
            free(option);
        } else {
            break;
        }
        option = get_value_from_tftp_header_at_index(tftp_oack_header, oack_index);
    }
    if (!is_blksize && parameters->s != 512) {
        // block size wasnt acknowledged by server, set it to default
        parameters->s = 512;
    }
    if (!is_timeout && parameters->t != -1) {
        // timeout wasnt acknowledged by server, set it to default
        parameters->t = -1;
    }
    free(option);
    return return_value;
}

char *get_value_from_tftp_header_at_index(char *tftp_header, int header_index) {
    char *return_value = malloc(strlen(tftp_header));
    int return_value_index = 0;
    for (int i = header_index; tftp_header[i] != '\0' ; ++i) {
        return_value[return_value_index++] = tftp_header[i];
    }
    if (return_value_index == 0) {
        return NULL;
    }
    return_value[return_value_index] = '\0';
    return return_value;
}
