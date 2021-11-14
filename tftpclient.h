//
// Created by musterino on 11/6/21.
//
#include "utilities.h"

#ifndef UNTITLED_TFTPCLIENT_H
#define UNTITLED_TFTPCLIENT_H

#define OP_CODE_RRQ     1
#define OP_CODE_WRQ     2
#define OP_CODE_DATA    3
#define OP_CODE_ACK     4
#define OP_CODE_ERROR   5
#define OP_CODE_OACK    6


int start_tftp_transfer(struct prms *parameters);
int send_tftp_data(int sock, struct sockaddr_in server, int block_num, char *data);
uint16_t get_block_num(char *tftp_header);
int check_oack_options(struct prms *parameters, char *tftp_header);
int send_tftp_ack(char *received_header);
int check_ack_packet(char *received_packet);

#endif //UNTITLED_TFTPCLIENT_H
