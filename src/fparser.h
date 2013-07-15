#ifndef FPARSER_H
#define FPARSER_H

#include "mrkdata.h"
#include "mrkcommon/array.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEX_SPACE    0x00000001
#define LEX_SEQIN    0x00000002
#define LEX_SEQOUT   0x00000004
#define LEX_TOKIN    0x00000008
#define LEX_TOKMID   0x00000010
#define LEX_TOKOUT   0x00000020
#define LEX_COMIN    0x00000040
#define LEX_COMMID   0x00000080
#define LEX_COMOUT   0x00000100
#define LEX_QSTRIN   0x00000200
#define LEX_QSTRMID  0x00000400
#define LEX_QSTROUT  0x00000800
#define LEX_QSTRESC  0x00001000

#define LEX_SEQ      (LEX_SEQIN | LEX_SEQOUT)
#define LEX_OUT      (LEX_TOKOUT | LEX_QSTROUT | LEX_COMOUT)
#define LEX_TOK      (LEX_TOKIN | LEX_TOKMID)
#define LEX_QSTR     (LEX_QSTRIN | LEX_QSTRMID)
#define LEX_COM      (LEX_COMIN | LEX_COMMID)

#define LEX_NEEDCB   (LEX_SEQ | LEX_TOKIN | LEX_TOKOUT | LEX_QSTRIN | LEX_QSTROUT)

#define LEXSTR(s) ( \
        s == LEX_SPACE ? "SPACE" : \
        s == LEX_SEQIN ? "SEQIN" : \
        s == LEX_SEQOUT ? "SEQOUT" : \
        s == LEX_TOKIN ? "TOKIN" : \
        s == LEX_TOKMID ? "TOKMID" : \
        s == LEX_TOKOUT ? "TOKOUT" : \
        s == LEX_COMIN ? "COMIN" : \
        s == LEX_COMMID ? "COMMID" : \
        s == LEX_COMOUT ? "COMOUT" : \
        s == LEX_QSTRIN ? "QSTRIN" : \
        s == LEX_QSTRMID ? "QSTRMID" : \
        s == LEX_QSTROUT ? "QSTROUT" : \
        s == LEX_QSTRESC ? "QSTRESC" : \
        "<unknown>" \
    )

typedef struct _fparser_datum {
    mrkdata_datum_t head;
    struct _fparser_datum *parent;
    unsigned char body[0];
} fparser_datum_t;

int
fparser_parse(int);

#ifdef __cplusplus
}
#endif

#endif

