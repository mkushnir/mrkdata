#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#define TRRET_DEBUG
#include "mrkcommon/dumpm.h"
#include "mrkcommon/util.h"

#include "diag.h"
#include "fparser_private.h"

#define BLOCKSZ (4096 * 8)
#define NEEDMORE 1

static int fparser_datum_init(fparser_datum_t *, mrkdata_tag_t);
static int fparser_datum_fini(fparser_datum_t *);
static int fparser_datum_form_add(fparser_datum_t *, fparser_datum_t *);
static int fparser_datum_dump(fparser_datum_t **, void *);


static int
tokenize(const unsigned char *buf, ssize_t buflen,
         int(*cb)(const unsigned char *, ssize_t, int, void *),
         void *udata)
{
    unsigned char ch;
    ssize_t i;
    int state = LEX_SPACE;

    D8(buf, buflen);

    for (i = 0; i < buflen; ++i) {
        ch = *(buf + i);
        if (ch == '(') {
            if (state & (LEX_SEQ | LEX_OUT | LEX_SPACE)) {

                state = LEX_SEQIN;

            } else if (state & LEX_TOK) {

                state = LEX_TOKOUT;
                /* extra call back */
                if (cb(buf, i, state, udata) != 0) {
                    TRRET(TOKENIZE + 1);
                }
                state = LEX_SEQIN;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }

        } else if (ch == ')') {
            if (state & (LEX_SEQ | LEX_OUT | LEX_SPACE)) {

                state = LEX_SEQOUT;

            } else if (state & LEX_TOK) {

                state = LEX_TOKOUT;
                /* extra call back */
                if (cb(buf, i, state, udata) != 0) {
                    TRRET(TOKENIZE + 1);
                }
                state = LEX_SEQOUT;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }

        } else if (ch == ' ' || ch == '\t') {

            if (state & (LEX_SEQ | LEX_OUT)) {

                state = LEX_SPACE;

            } else if (state & LEX_TOK) {

                state = LEX_TOKOUT;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }

        } else if (ch == '\r' || ch == '\n') {

            if (state & (LEX_SEQ | LEX_OUT)) {

                state = LEX_SPACE;

            } else if (state & LEX_TOK) {

                state = LEX_TOKOUT;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COM) {

                state = LEX_COMOUT;

            } else {
                /* noop */
            }

        } else if (ch == '"') {

            if (state & (LEX_SEQ | LEX_OUT | LEX_SPACE)) {

                state = LEX_QSTRIN;

            } else if (state & LEX_QSTR) {

                state = LEX_QSTROUT;

            } else if (state & LEX_QSTRESC) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }

        } else if (ch == '\\') {
            if (state & LEX_QSTR) {

                state = LEX_QSTRESC;

            } else if (state & LEX_QSTRESC) {

                state = LEX_QSTRMID;

            } else {
                /* noop */
            }

        } else if (ch == ';') {

            if (state & (LEX_SEQ | LEX_OUT | LEX_SPACE)) {

                state = LEX_COMIN;

            } else if (state & LEX_TOK) {

                state = LEX_TOKOUT;
                /* extra call back */
                if (cb(buf, i, state, udata) != 0) {
                    TRRET(TOKENIZE + 1);
                }
                state = LEX_COMIN;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }

        } else {
            if (state & (LEX_SEQ | LEX_SPACE | LEX_OUT)) {

                state = LEX_TOKIN;

            } else if (state & LEX_TOKIN) {

                state = LEX_TOKMID;

            } else if (state & (LEX_QSTRIN | LEX_QSTRESC)) {

                state = LEX_QSTRMID;

            } else if (state & LEX_COMIN) {

                state = LEX_COMMID;

            } else {
                /* noop */
            }
        }

        if (state & LEX_NEEDCB) {
            if (cb(buf, i, state, udata) != 0) {
                TRRET(TOKENIZE + 1);
            }
        }
    }

    return (NEEDMORE);
}


static void
_unescape(unsigned char *dst, const unsigned char *src, ssize_t sz)
{
    ssize_t i;
    int j;
    char ch;

    for (i = 0, j = 0; i < sz; ++i, ++j) {
        ch = *(src + i);
        if (ch == '\\') {
            ++i;
            if (i < sz) {
                *(dst + j) = *(src + i);
            }
        } else {
            *(dst + j) = ch;
        }
    }
    *(dst + j) = '\0';
}

static int
_cb(const unsigned char *buf, ssize_t idx, int state, void *udata)
{
    struct tokenizer_ctx *ctx = udata;
    fparser_datum_t *dat;

    if (state == LEX_TOKIN) {
        ctx->tokstart = buf + idx;

    } else if (state == LEX_QSTRIN) {
        ctx->tokstart = buf + idx + 1;

    } else if (state & (LEX_QSTROUT)) {
        ssize_t sz = buf + idx - ctx->tokstart;

        if ((dat = malloc(sizeof(fparser_datum_t) + sz + 1)) == NULL) {
            perror("malloc");
            return (1);
        }
        if (fparser_datum_init(dat, MRKDATA_STR8) != 0) {
            perror("fparser_datum_init");
            return (1);
        }
        dat->head.value.sz8 = (uint8_t)sz;

        _unescape(dat->body, ctx->tokstart, sz);

        if (fparser_datum_form_add(ctx->form, dat) != 0) {
            perror("fparser_datum_form_add");
            return (1);
        }

        //LTRACE(ctx->indent, " '%s'", dat->body);

    } else if (state & (LEX_TOKOUT)) {
        unsigned char ch = ctx->tokstart[0];
        ssize_t sz = buf + idx - ctx->tokstart;

        if (ch == '+' || ch == '-' || (ch >= '0' && ch <= '9')) {
            int i, godouble = 0;

            /* number */

            if ((dat = malloc(sizeof(fparser_datum_t))) == NULL) {
                perror("malloc");
                return (1);
            }

            /* first check for double */
            for (i = 0; i < sz; ++i) {
                if (ctx->tokstart[i] == '.') {
                    godouble = 1;
                    break;
                }
            }

            if (godouble) {
                if (fparser_datum_init(dat, MRKDATA_DOUBLE) != 0) {
                    perror("fparser_datum_init");
                    return (1);
                }
                dat->head.value.d = strtod((const char *)ctx->tokstart, NULL);
            } else {
                if (fparser_datum_init(dat, MRKDATA_INT64) != 0) {
                    perror("fparser_datum_init");
                    return (1);
                }
                dat->head.value.i64 = (int64_t)strtoll(
                        (const char *)ctx->tokstart, NULL, 10);
            }
        } else {
            ssize_t sz = buf + idx - ctx->tokstart;

            if ((dat = malloc(sizeof(fparser_datum_t) + sz + 1)) == NULL) {
                perror("malloc");
                return (1);
            }
            if (fparser_datum_init(dat, MRKDATA_STR8) != 0) {
                perror("fparser_datum_init");
                return (1);
            }
            dat->head.value.sz8 = (uint8_t)sz;

            memcpy(dat->body, ctx->tokstart, sz);
            *(dat->body + sz) = '\0';
        }

        if (fparser_datum_form_add(ctx->form, dat) != 0) {
            perror("fparser_datum_form_add");
            return (1);
        }

        //LTRACE(ctx->indent, " %s", dat->body);

    } else if (state & LEX_SEQIN) {
        ++(ctx->indent);

        if ((dat = malloc(sizeof(fparser_datum_t) +
                          sizeof(array_t))) == NULL) {
            perror("malloc");
            return (1);
        }
        if (fparser_datum_init(dat, MRKDATA_SEQ) != 0) {
            perror("fparser_datum_init");
            return (1);
        }

        if (fparser_datum_form_add(ctx->form, dat) != 0) {
            perror("fparser_datum_form_add");
            return (1);
        }

        ctx->form = dat;


    } else if (state & LEX_SEQOUT) {

        if (ctx->indent <= 0) {
            return (1);
        }
        --(ctx->indent);

        if (ctx->form->parent == NULL) {
            /* root */
            assert(0);
        }
        ctx->form = ctx->form->parent;
    }

    return (0);
}

static int
fparser_datum_fini(fparser_datum_t *dat)
{
    if (dat->head.spec->tag == MRKDATA_SEQ) {
        array_iter_t it;
        fparser_datum_t **o;

        array_t *form = (array_t *)(&dat->body);

        for (o = array_first(form, &it);
             o != NULL;
             o = array_next(form, &it)) {

            if (*o != NULL) {
                fparser_datum_fini(*o);
                free(*o);
                *o = NULL;
            }
        }
        array_fini(form);
    }
    mrkdata_spec_destroy((mrkdata_spec_t **)&dat->head.spec);
    return (0);
}

static int
fparser_datum_dump(fparser_datum_t **dat, UNUSED void *udata)
{
    fparser_datum_t *rdat = *dat;

    //TRACE("tag=%d", rdat->head.spec->tag);
    if (rdat->head.spec->tag == MRKDATA_SEQ) {

        array_t *form = (array_t *)(&rdat->body);

        TRACE("form len %ld", form->elnum);

        array_traverse(form, (array_traverser_t)fparser_datum_dump, udata);

    } else if (rdat->head.spec->tag == MRKDATA_STR8) {
        TRACE("STR8 '%s'", rdat->body);

    } else if (rdat->head.spec->tag == MRKDATA_INT64) {
        TRACE("INT64 '%ld'", rdat->head.value.i64);

    } else if (rdat->head.spec->tag == MRKDATA_DOUBLE) {
        TRACE("DOUBLE '%f'", rdat->head.value.d);
    }

    return (0);
}


static int
fparser_datum_init(fparser_datum_t *dat, mrkdata_tag_t tag)
{
    //TRACE("tag=%d value=%ld", tag, value);
    dat->head.spec = mrkdata_make_spec(tag);
    dat->parent = NULL;

    if (tag == MRKDATA_SEQ) {

        array_t *form = (array_t *)(&dat->body);

        if (array_init(form,
                       sizeof(fparser_datum_t *), 0,
                       NULL, NULL) != 0) {

            TRRET(FPARSER_DATUM_INIT + 1);
        }

    }

    return (0);
}

static int
fparser_datum_form_add(fparser_datum_t *parent, fparser_datum_t *dat)
{
    assert(parent->head.spec->tag == MRKDATA_SEQ);

    //TRACE("Adding %d to %d", dat->head.spec->tag, parent->head.spec->tag);

    array_t *form = (array_t *)parent->body;
    fparser_datum_t **entry;

    if ((entry = array_incr(form)) == NULL) {
        TRRET(FPARSER_DATUM_FORM_ADD + 1);
    }
    *entry = dat;
    dat->parent = parent;
    return (0);
}

int
fparser_parse(int fd)
{
    int res;
    ssize_t nread;
    unsigned char buf[BLOCKSZ];
    struct tokenizer_ctx ctx;
    fparser_datum_t *root;

    ctx.indent = 0;
    res = 0;

    if ((root = malloc(sizeof(fparser_datum_t) + sizeof(array_t))) == NULL) {
        TRRET(FPARSER_PARSE + 1);
    }

    if (fparser_datum_init(root, MRKDATA_SEQ) != 0) {
        TRRET(FPARSER_PARSE + 2);
    }

    ctx.form = root;

    while (1) {
        if ((nread = read(fd, buf, BLOCKSZ)) <= 0) {

            if (nread < 0) {
                TRRET(FPARSER_PARSE + 3);

            } else {
                break;
            }

        }

        if ((res = tokenize(buf, nread, _cb, &ctx)) == NEEDMORE) {
            continue;
        }
    }


    TRACE("indent=%d", ctx.indent);

    fparser_datum_dump(&root, NULL);

    fparser_datum_fini(root);
    free(root);
    root = NULL;

    return (res);
}
/*
 * vim:softtabstop=4
 */
