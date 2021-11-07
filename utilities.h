//
// Created by musterino on 11/2/21.
//
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

#ifndef UNTITLED_UTILITIES_H
#define UNTITLED_UTILITIES_H
#define BUFFER 1024

struct prms {
    int RW;         // read - 0 | write - 1
    char *d; // filepath
    int t;         // timeout (s)
    int s;         // blocksize
    bool m;         // multicast
    char *c;         // mode - ascii - false | binary - true
    char *address;  // interface ip address
    int port;       // port
};

int parse_parameters(struct prms *params, char *input);


#endif //UNTITLED_UTILITIES_H
