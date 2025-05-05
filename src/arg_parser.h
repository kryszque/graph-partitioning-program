#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <argp.h>
/* This structure is used by main to communicate with parse_opt. */
struct arguments {
    char *input;
    int parts;
    double margin;
    char *output;
    char *format;
};

int parse_arguments(int argc, char **argv);
struct arguments* get_arguments();

#endif
