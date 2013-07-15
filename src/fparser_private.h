#ifndef FPARSER_PRIVATE_H
#define FPARSER_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct _fparser_datum;

struct tokenizer_ctx {
    const unsigned char *tokstart;
    int indent;
    struct _fparser_datum *form;
};

#ifdef __cplusplus
}
#endif

#include "fparser.h"
#endif
