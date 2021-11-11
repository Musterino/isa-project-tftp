//
// Created by musterino on 11/6/21.
//

#include <unistd.h>
#include "tftpclient.h"

int start_tftp_transfer(struct prms *parameters) {
    int sock, msg_size, block_num;
    struct sockaddr_in server, from;
    char *packet, buffer[BUFFER], byte_block_num[BUFFER];
    socklen_t len;
    FILE *filep;

    // create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        err(1, "Unable to create socket!");

    // erase server structure
    memset(&server, 0, sizeof(server));
    // fill server structure
    server.sin_family = AF_INET;
    inet_aton(parameters->address, &server.sin_addr);
    server.sin_port = parameters->port;

    block_num = 0;
    // set opcode
    if (parameters->RW) {
        *(u_int16_t *) buffer = htons(OP_CODE_RRQ);  // set 2 byte RRQ opcode to add to packet
    } else {
        *(u_int16_t *) buffer = htons(OP_CODE_WRQ);  // set 2 byte WRQ opcode to add to packet
    }
    // set the rest of tftp header
    packet = buffer + 2;                        // add opcode to packet and move pointer to the end of opcode
    strcpy(packet, parameters->d);              // add directory string to packet
    packet += strlen(parameters->d) + 1;        // move pointer to end of directory string + 1 byte of zeros
    strcpy(packet, parameters->c);              // add mode string to packet
    packet += strlen(parameters->c) + 1;        // move pointer to end of mode string + 1 byte of zeros

    // send request to server
    msg_size = sendto(sock, buffer, packet-buffer, 0, (struct sockaddr *) &server, sizeof(server));
    // get acknowledgement packet
    msg_size = recvfrom(sock, buffer, BUFFER, 0, (struct sockaddr *)&server, &len);

    if (*(uint16_t *)buffer == OP_CODE_ERROR) { // error opcode = 5
        // TODO: continue after receiving error message maybe
        fprintf(stderr, "Error message received:\n%s", buffer);
        return 1;
    }

    // start file transfer
    if (parameters->RW) {
        // WRITE
        int total_size, remaining_size;
        char data[parameters->s + 5];

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

            // set tftp packet header
            packet = NULL;
            // opcode
            *(u_int16_t *) buffer = htons(OP_CODE_DATA);
            packet = buffer + 2;
            // filename
            strcpy(packet, parameters->d);
            packet += strlen(parameters->d) + 1;
            // block number
            *(uint16_t *) byte_block_num = htons(block_num++);
            strcpy(packet, byte_block_num);
            packet += strlen(byte_block_num);
            // data
            strcpy(packet, data);

            // send data packet
            msg_size = sendto(sock, buffer, packet-buffer, 0, (struct sockaddr *) &server, len);
            // get acknowledgement packet
            msg_size = recvfrom(sock, buffer, BUFFER, 0, (struct sockaddr *)&server, &len);
        }


    } else {
        // READ
        do {
             if (*(uint16_t *) buffer == OP_CODE_DATA) {
                // write data to file, start after opcode and block number - 4 bytes
                write(1, buffer+4, msg_size-4);
                // change opcode to acknowledgement
                *(uint16_t *) buffer = htons(OP_CODE_ACK);
                // send ack packet
                sendto(sock, buffer, 4, 0, (struct sockaddr *) &server, len);
            }
        } while (msg_size == 516);
    }

    return 0;
}