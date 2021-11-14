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
#include "tftpclient.h"

int start_tftp_transfer(struct prms *parameters) {
    int sock, msg_size, header_len;
    uint16_t block_num, opcode;
    struct sockaddr_in default_server, server;
    char tftp_header[BUFFER];
    socklen_t len;
    FILE *filep;

    // create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        err(1, "Unable to create socket!");

    printf("Setting up server structure...\n");
    // erase server structure
    memset(&default_server, 0, sizeof(default_server));
    memset(&server, 0, sizeof(server));
    // fill server structure
    default_server.sin_family = AF_INET;
    inet_aton(parameters->address, &default_server.sin_addr);
    default_server.sin_port = htons(parameters->port);

    printf("Creating TFTP header...\n");

    // create tftp header

    // set opcode
    if (parameters->RW) {
        opcode = OP_CODE_WRQ;  // set 2 byte WRQ opcode to add to packet
    } else {
        opcode = OP_CODE_RRQ;  // set 2 byte RRQ opcode to add to packet
    }

    // create tftp header
    header_len = sprintf(tftp_header, "%c%c%s%c%s%c", 0, opcode, parameters->d, 0, parameters->c, 0);
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

    printf("TFTP header created!\n");


    printf("Sending request to server...\n");
    // send request to server
    msg_size = sendto(sock, tftp_header, header_len, 0, (struct sockaddr *) &default_server, sizeof(server));
    printf("Request sent! size: %d\n", msg_size);

    printf("Waiting for ack packet...\n");

    // get acknowledgement packet
    for (int try = 0; try < 4; ++try) {
        msg_size = recvfrom(sock, tftp_header, BUFFER, 0, (struct sockaddr *)&server, &len);
        if (msg_size < 0) {
            fprintf(stderr, "Failed recvfrom!\n");
            return 1;
        } else if (ntohs(server.sin_port) == 69) {
            printf("Not received ack!\n");
            continue;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ACK){
            printf("Valid ack!\n");
            break;
        } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_OACK){
            int oack_options = check_oack_options(parameters, tftp_header);
            printf("Valid option ack!\n");
            break;
        }
    }

    if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ERROR) { // error opcode = 5
        // TODO: continue after receiving error message maybe
        fprintf(stderr, "Error message received:\n%s\n", tftp_header+4);
        return 1;
    }


    // start file transfer
    if (parameters->RW) {
                        //================= WRITE DATA =====================//
        int total_size, remaining_size;
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

        // get size of file
        fseek(filep, 0L, SEEK_END);
        total_size = remaining_size = ftell(filep);
        rewind(filep);

        block_num = 1;

        // start reading and sending file data
        while (remaining_size > 0) {
            // read file data
            if (parameters->s < remaining_size) {
                // remaining size > set size so fill the whole packet
                fread(data, parameters->s, sizeof(char), filep);
                // add end of string
                data[parameters->s] = '\0';
            } else {
                // remaining size < set size -> last packet to be sent
                fread(data, remaining_size, sizeof(char), filep);
                // add end of string
                data[remaining_size] = '\0';
            }

            // subtract size of data read from remaining size
            remaining_size -= parameters->s;

            // send data packet
            if (send_tftp_data(sock, server, block_num, data) <= 0) {
                fprintf(stderr, "Error sending data packet!\n");
                return 1;
            }

            // get acknowledgement packet
            for (int try = 0; try < 4; ++try) {
                msg_size = recvfrom(sock, tftp_header, BUFFER, 0, (struct sockaddr *)&server, &len);
                if (msg_size < 0) {
                    fprintf(stderr, "Failed recvfrom!\n");
                    return 1;
                } else if (ntohs(*(uint16_t *)tftp_header) == OP_CODE_ACK) {
                    if (get_block_num(tftp_header) == block_num)
                        break;
                }
            }
            block_num++;
        }


    } else {
                    //================= READ DATA =====================//
        if (parameters->is_binary) {
            // open file read binary
            filep = fopen(parameters->d, "wb");
        } else {
            // open file read ascii
            filep = fopen(parameters->d, "w");
        }
        if (filep == NULL) {
            fprintf(stderr, "Could not open file!");
            return 1;
        }

        int last_block_number = 0;
        bool is_last = false;
        while (1) {
            if (msg_size == (parameters->s + 4) || is_last) {
                block_num = get_block_num(tftp_header);
                printf("Reading data block: %d\n", block_num);
                if (ntohs(*(uint16_t *) tftp_header) == OP_CODE_DATA) {
                    // check if data packet isn't duplicate
                    if (last_block_number != block_num) {
                        last_block_number = block_num;
                        // write data to file, start after opcode and block number - 4 bytes
                        fwrite(tftp_header+4, sizeof(char), msg_size-4, filep);
                    }
                    // change opcode to acknowledgement
                    *(uint16_t *) tftp_header = htons(OP_CODE_ACK);
                    // send ack packet
                    sendto(sock, tftp_header, 4, 0, (struct sockaddr *) &server, len);
                } else {
                    printf("Bad header: %d\n", *(uint16_t *) tftp_header);
                }
                // if last block is acknowledged break out of loop
                if (is_last)
                    break;
                // get next data packet
                msg_size = recvfrom(sock, tftp_header, BUFFER, 0, (struct sockaddr *)&server, &len);
                // last packet with size < set_size (512 default) + 4 is terminating transfer
                if (msg_size > 0 && msg_size < (parameters->s + 4)) {
                    is_last = true;
                }
            } else {
                break;
            }
        }
    }

    // close socket
    close(sock);
    return 0;
}


int send_tftp_data(int sock, struct sockaddr_in server, int block_num, char *data) {
    int header_len;
    int len = sizeof(server);
    uint16_t opcode;
    char tftp_header[BUFFER], byte_block_num[2];

    // create tftp data header
    // set opcode
    opcode = htons(OP_CODE_DATA);
    // add opcode to header
    memcpy(tftp_header, &opcode, 2);
    // update header length
    header_len = 2;
    // add block number
    *(uint16_t *) byte_block_num = htons(block_num);
    memcpy(tftp_header + header_len, &byte_block_num, 2);
    // update header length
    header_len += 2;
    // add data to header
    strcpy(tftp_header + header_len, data);
    // update header length
    header_len += strlen(data);
    // send data packet
    return sendto(sock, tftp_header, header_len, 0, (struct sockaddr *) &server, len);
}

uint16_t get_block_num(char *tftp_header) {
    char byte_block_num[2];
    // get block number
    byte_block_num[0] = tftp_header[2];
    byte_block_num[1] = tftp_header[3];
    return ntohs(*(uint16_t *)byte_block_num);
}

int check_oack_options(struct prms *parameters, char *tftp_oack_header) {
    int oack_index = 2;     // skip opcode (2 bytes)
    char option[BUFFER];
    int return_value = 0;
    char *token = strtok(tftp_oack_header + oack_index, "0");
    while (token != NULL) {
        if (strcmp(token, "blksize") == 0) {
            token = strtok(NULL, "0");
            int size = strtol(tftp_oack_header + oack_index + strlen("blksize"), NULL, 10);
            if (strcmp(token, "") != 0) {
                // different block size as requested, transfer with acknowledged
                strcpy("", token);
                parameters->s = strtol(token, NULL, 10);
                return_value += 1;
            }
        } else if (strcmp(token, "tsize") == 0) {
            return_value += 2;
        } else if (strcmp(token, "timeout") == 0) {
            return_value += 4;
        }
        token = strtok(token, "0");
    }
    return return_value;
}
