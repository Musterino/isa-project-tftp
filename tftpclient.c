//
// Created by musterino on 11/6/21.
//

#include <unistd.h>
#include "tftpclient.h"

int start_tftp_transfer(struct prms *parameters) {
    int sock, msg_size, block_num;
    struct sockaddr_in server, from;
    char *packet, buffer[BUFFER];
    socklen_t len;

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

    if (parameters->RW) {
        // WRITE
    } else {
        // READ
        *(u_int16_t *)buffer = htons(OP_CODE_RRQ);  // create 2 byte opcode to add to packet
        packet = buffer + 2;                        // add opcode to packet and move pointer to the end of opcode
        strcpy(packet, parameters->d);              // add directory string to packet
        packet += strlen(parameters->d) + 1;        // move pointer to end of directory string + 1 byte of zeros
        strcpy(packet, parameters->c);              // add mode string to packet
        packet += strlen(parameters->c) + 1;        // move pointer to end of mode string + 1 byte of zeros

        msg_size = sendto(sock, buffer, packet-buffer, 0, (struct sockaddr *) &server, sizeof(server));

        // receive data
        do {
            msg_size = recvfrom(sock, buffer, BUFFER, 0, (struct sockaddr *)&server, &len);
            if (*(uint16_t *)buffer == OP_CODE_ERROR) { // error opcode = 5
                // TODO: continue after receiving error message maybe
                fprintf(stderr, "Error message received:\n%s", buffer);
                return 1;
            } else if (*(uint16_t *) buffer == OP_CODE_DATA) {
                // write data to file, start after opcode and block number - 4 bytes
                write(1, buffer+4, msg_size-4);
                // change opcode to acknowledgement
                *(uint16_t *) buffer = htons(OP_CODE_ACK);
                // send 4 byte ack packet
                sendto(sock, buffer, 4, 0, (struct sockaddr *) &server, len);
            }
        } while (msg_size == 516);
    }

    return 0;
}