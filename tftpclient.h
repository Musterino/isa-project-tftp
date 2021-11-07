//
// Created by musterino on 11/6/21.
//
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <err.h>

#include "utilities.h"

#ifndef UNTITLED_TFTPCLIENT_H
#define UNTITLED_TFTPCLIENT_H

#define OP_CODE_RRQ 1
#define OP_CODE_WRQ 2
#define OP_CODE_DATA 3
#define OP_CODE_ACK 4
#define OP_CODE_ERROR 5


int start_tftp_transfer(struct prms *parameters);

#endif //UNTITLED_TFTPCLIENT_H
