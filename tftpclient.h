//
// Created by musterino on 11/6/21.
//
#include "utilities.h"

#ifndef UNTITLED_TFTPCLIENT_H
#define UNTITLED_TFTPCLIENT_H

#define OK              0
#define IS_OPTIONS      10
#define ERROR           99

#define OP_CODE_RRQ     1
#define OP_CODE_WRQ     2
#define OP_CODE_DATA    3
#define OP_CODE_ACK     4
#define OP_CODE_ERROR   5
#define OP_CODE_OACK    6



int start_tftp_transfer(struct prms *parameters);
int send_tftp_data(int sock, struct sockaddr server, int block_num, char *data);
int send_tftp_ack(int sock, struct sockaddr server, int block_num);
int check_tftp_ack(int block_num, char *tftp_header, struct prms *parameters, struct sockaddr server);
uint16_t get_block_num(char *tftp_header);
char *get_value_from_tftp_header_at_index(char *tftp_header, int header_index);
int check_oack_options(struct prms *parameters, char *tftp_header);
in_port_t get_in_port(struct sockaddr *sa);
int check_ack_packet(char *received_packet);

#endif //UNTITLED_TFTPCLIENT_H
