#include <assert.h>
#include <netinet/in.h>
#include <sys/endian.h>

#include "mrkcommon/list.h"
#include "mrkcommon/array.h"
#define TRRET_DEBUG
#include "mrkcommon/dumpm.h"
#include "mrkcommon/util.h"

#include "mrkcommon/memdebug.h"
MEMDEBUG_DECLARE(mrkdata);

#include "diag.h"
#include "mrkdata_private.h"

#define MRKDATA_MFLAG_INITIALIZED (0x01)
static unsigned mflags = 0;

/* sync with enum _mrkdata_tag */
static ssize_t
tag_sz[] = {
    sizeof(uint8_t),    /* UINT8 */
    sizeof(int8_t),     /* INT8 */
    sizeof(uint16_t),   /* UINT16 */
    sizeof(int16_t),    /* INT16 */
    sizeof(uint32_t),   /* UINT32 */
    sizeof(int32_t),    /* INT32 */
    sizeof(uint64_t),   /* UINT64 */
    sizeof(int64_t),    /* INT64 */
    sizeof(double),     /* DOUBLE */
    sizeof(uint8_t),    /* STR8 sz */
    sizeof(uint16_t),   /* STR16 sz */
    sizeof(uint32_t),   /* STR32 sz */
    sizeof(uint64_t),   /* STR64 sz */
    sizeof(uint64_t),   /* STRUCT sz */
    sizeof(uint64_t),   /* SEQ sz */
};

static mrkdata_spec_t builtin_specs[MRKDATA_BUILTIN_TAG_END];
static array_t specs;

#define EXPECT_SZ(t) ((ssize_t)(sizeof(char) + tag_sz[t]))
#define EXPECT_EXTERNAL (-1)

static int datum_init(mrkdata_datum_t *);

static int
null_pointer_initializer(void **p)
{
    *p = NULL;
    return 0;
}

static void
mrkdata_datum_adjust_packsz(mrkdata_datum_t *dat, ssize_t sz)
{
    dat->packsz += sz;
    if (dat->spec->tag == MRKDATA_STRUCT || dat->spec->tag == MRKDATA_SEQ) {
        dat->value.sz64 += sz;
    }

    if (dat->parent != NULL) {
        mrkdata_datum_adjust_packsz(dat->parent, sz);
    }
}

ssize_t
mrkdata_pack_datum(const mrkdata_datum_t *dat, unsigned char *buf, ssize_t sz)
{

    if (dat->packsz > sz) {
        return 0;
    }

    *buf = dat->spec->tag;
    ++buf;
    --sz;

    switch (dat->spec->tag) {
        ssize_t nwritten;
        mrkdata_datum_t **field;
        list_iter_t it;

        case MRKDATA_UINT8:
            *((uint8_t *)buf) = dat->value.u8;
            break;

        case MRKDATA_INT8:
            *((int8_t *)buf) = dat->value.i8;
            break;

        case MRKDATA_UINT16:
            *((uint16_t *)buf) = htons(dat->value.u16);
            break;

        case MRKDATA_INT16:
            *((int16_t *)buf) = htons(dat->value.i16);
            break;

        case MRKDATA_UINT32:
            *((uint32_t *)buf) = htonl(dat->value.u32);
            break;

        case MRKDATA_INT32:
            *((int32_t *)buf) = htonl(dat->value.i32);
            break;

        case MRKDATA_UINT64:
            *((uint64_t *)buf) = htobe64(dat->value.u64);
            break;

        case MRKDATA_INT64:
            *((int64_t *)buf) = htobe64(dat->value.i64);
            break;

        case MRKDATA_DOUBLE:
            *((double *)buf) = dat->value.d;
            break;

        case MRKDATA_STR8:
            *((int8_t *)buf) = dat->value.sz8;
            buf += sizeof(int8_t);
            memcpy(buf, dat->data.str, dat->value.sz8);
            break;

        case MRKDATA_STR16:
            *((int16_t *)buf) = htons(dat->value.sz16);
            buf += sizeof(int16_t);
            memcpy(buf, dat->data.str, dat->value.sz16);
            break;

        case MRKDATA_STR32:
            *((int32_t *)buf) = htonl(dat->value.sz32);
            buf += sizeof(int32_t);
            memcpy(buf, dat->data.str, dat->value.sz32);
            break;

        case MRKDATA_STR64:
            *((int64_t *)buf) = htobe64(dat->value.sz64);
            buf += sizeof(int64_t);
            memcpy(buf, dat->data.str, dat->value.sz64);
            break;

        case MRKDATA_STRUCT:
        case MRKDATA_SEQ:
            *((int64_t *)buf) = htobe64(dat->value.sz64);
            buf += sizeof(int64_t);
            sz -= sizeof(int64_t);
            for (field = list_first(&dat->data.fields, &it);
                 field != NULL;
                 field = list_next(&dat->data.fields, &it)) {

                if (sz <= 0) {
                    return 0;
                }

                if ((nwritten = mrkdata_pack_datum(*field, buf, sz)) == 0) {
                }

                buf += nwritten;
                sz -= nwritten;
            }

            break;

        default:
            return 0;

    }
    return dat->packsz;
}

ssize_t
mrkdata_unpack_buf(const mrkdata_spec_t *spec,
                   const unsigned char *buf,
                   ssize_t sz,
                   mrkdata_datum_t **pdat)
{
    mrkdata_tag_t tag;
    ssize_t valsz;
    mrkdata_datum_t *dat;

    assert(pdat != NULL);

    if (sz == 0) {
        return 0;
    }

    if (*pdat == NULL) {
        *pdat = malloc(sizeof(mrkdata_datum_t));
        datum_init(*pdat);

    }
    dat = *pdat;

    tag = (mrkdata_tag_t)(*buf);

    if (tag != spec->tag) {
        return 0;
    }
    (*pdat)->spec = spec;

    valsz = EXPECT_SZ(spec->tag);

    if (sz < valsz) {
        return 0;
    }

    /* must be *after* the above test */
    buf += sizeof(char);
    sz -= sizeof(char);

    dat->packsz = valsz;

    switch (tag) {
        ssize_t nread;
        mrkdata_spec_t **field_spec;
        list_iter_t it;

    case MRKDATA_UINT8:
        dat->value.u8 = *buf;
        buf += sizeof(uint8_t);
        sz -= sizeof(uint8_t);
        break;

    case MRKDATA_INT8:
        dat->value.i8 = *buf;
        buf += sizeof(int8_t);
        sz -= sizeof(int8_t);
        break;

    case MRKDATA_STR8:
        dat->value.sz8 = *buf;
        buf += sizeof(int8_t);
        sz -= sizeof(int8_t);
        if (sz < dat->value.sz8) {
            return 0;
        }
        dat->packsz += dat->value.sz8;
        if ((dat->data.str = malloc(dat->value.sz8)) == NULL) {
            FAIL("malloc");
        }
        memcpy(dat->data.str, buf, dat->value.sz8);
        buf += dat->value.sz8;
        sz -= dat->value.sz8;
        break;

    case MRKDATA_UINT16:
        dat->value.u16 = ntohs(*((uint16_t *)buf));
        buf += sizeof(uint16_t);
        sz -= sizeof(uint16_t);
        break;

    case MRKDATA_INT16:
        dat->value.i16 = (int16_t)ntohs(*((uint16_t *)buf));
        buf += sizeof(int16_t);
        sz -= sizeof(int16_t);
        break;

    case MRKDATA_STR16:
        dat->value.sz16 = ntohs(*((int16_t *)buf));
        buf += sizeof(int16_t);
        sz -= sizeof(int16_t);
        if (sz < dat->value.sz16) {
            return 0;
        }
        dat->packsz += dat->value.sz16;
        if ((dat->data.str = malloc(dat->value.sz16)) == NULL) {
            FAIL("malloc");
        }
        memcpy(dat->data.str, buf, dat->value.sz16);
        buf += dat->value.sz16;
        sz -= dat->value.sz16;
        break;

    case MRKDATA_UINT32:
        dat->value.u32 = ntohl(*((uint32_t *)buf));
        buf += sizeof(uint32_t);
        sz -= sizeof(uint32_t);
        break;

    case MRKDATA_INT32:
        dat->value.i32 = (int32_t)ntohl(*((uint32_t *)buf));
        buf += sizeof(int32_t);
        sz -= sizeof(int32_t);
        break;

    case MRKDATA_STR32:
        dat->value.sz32 = ntohl(*((int32_t *)buf));
        buf += sizeof(int32_t);
        sz -= sizeof(int32_t);
        if (sz < dat->value.sz32) {
            return 0;
        }
        dat->packsz += dat->value.sz32;
        if ((dat->data.str = malloc(dat->value.sz32)) == NULL) {
            FAIL("malloc");
        }
        memcpy(dat->data.str, buf, dat->value.sz32);
        buf += dat->value.sz32;
        sz -= dat->value.sz32;
        break;

    case MRKDATA_UINT64:
        dat->value.u64 = be64toh(*((uint64_t *)buf));
        buf += sizeof(uint64_t);
        sz -= sizeof(uint64_t);
        break;

    case MRKDATA_INT64:
        dat->value.i64 = (int64_t)be64toh(*((uint64_t *)buf));
        buf += sizeof(int64_t);
        sz -= sizeof(int64_t);
        break;

    case MRKDATA_STR64:
        dat->value.sz64 = be64toh(*((int64_t *)buf));
        buf += sizeof(int64_t);
        sz -= sizeof(int64_t);
        if (sz < dat->value.sz64) {
            return 0;
        }
        dat->packsz += dat->value.sz64;
        if ((dat->data.str = malloc(dat->value.sz64)) == NULL) {
            FAIL("malloc");
        }
        memcpy(dat->data.str, buf, dat->value.sz64);
        buf += dat->value.sz64;
        sz -= dat->value.sz64;
        break;

    case MRKDATA_STRUCT:
        dat->value.sz64 = be64toh(*((int64_t *)buf));
        buf += sizeof(int64_t);
        sz -= sizeof(int64_t);

        if (sz < dat->value.sz64) {
            return 0;
        }

        dat->packsz += dat->value.sz64;

        if (list_init(&dat->data.fields, sizeof(mrkdata_datum_t *), 0,
                      (list_initializer_t)null_pointer_initializer,
                      (list_finalizer_t)mrkdata_datum_destroy) != 0) {
            FAIL("list_init");
        }

        for (field_spec = list_first(&spec->fields, &it);
             field_spec != NULL;
             field_spec = list_next(&spec->fields, &it)) {

            mrkdata_datum_t **field_dat;

            if (sz <= 0) {
                return 0;
            }

            if ((field_dat = list_incr(&dat->data.fields)) == NULL) {
                FAIL("list_incr");
            }

            if ((nread = mrkdata_unpack_buf(*field_spec,
                                            buf,
                                            sz,
                                            field_dat)) == 0) {
                return 0;
            }

            buf += nread;
            sz -= nread;
        }

        break;


    case MRKDATA_SEQ:
        dat->value.sz64 = be64toh(*((int64_t *)buf));
        buf += sizeof(int64_t);
        sz -= sizeof(int64_t);

        if (sz < dat->value.sz64) {
            return 0;
        }

        if (spec->fields.elnum != 1) {
            return 0;
        }

        dat->packsz += dat->value.sz64;

        if (list_init(&dat->data.fields, sizeof(mrkdata_datum_t *), 0,
                      (list_initializer_t)null_pointer_initializer,
                      (list_finalizer_t)mrkdata_datum_destroy) != 0) {
            FAIL("list_init");
        }

        field_spec = list_first(&spec->fields, &it);

        nread = 0;
        while (nread < dat->value.sz64 && sz > 0) {
            mrkdata_datum_t **field_dat;
            ssize_t nread_single;

            if ((field_dat = list_incr(&dat->data.fields)) == NULL) {
                FAIL("list_incr");
            }

            if ((nread_single = mrkdata_unpack_buf(*field_spec,
                                            buf,
                                            sz,
                                            field_dat)) == 0) {
                return 0;
            }

            buf += nread_single;
            sz -= nread_single;
            nread += nread_single;

        }

        break;


    default:
        assert(0);
    }

    return dat->packsz;
}


/* spec */
static int
_mrkdata_spec_dump(mrkdata_spec_t *spec, int lvl)
{
    LTRACE(lvl, "<spec tag=%s>", MRKDATA_TAG_STR(spec->tag));
    if (spec->tag == MRKDATA_STRUCT || spec->tag == MRKDATA_SEQ) {
        list_iter_t it;
        mrkdata_spec_t **field;
        for (field = list_first(&spec->fields, &it);
             field != NULL;
             field = list_next(&spec->fields, &it)) {
            _mrkdata_spec_dump(*field, lvl + 1);
        }
    }
    return 0;
}

int
mrkdata_spec_dump(mrkdata_spec_t *spec)
{
    return _mrkdata_spec_dump(spec, 0);
}

int
mrkdata_spec_destroy(mrkdata_spec_t **spec)
{
    if (*spec != NULL) {

        if ((*spec)->tag == MRKDATA_STRUCT || (*spec)->tag == MRKDATA_SEQ) {
            list_fini(&(*spec)->fields);
            free(*spec);
        }

        *spec = NULL;
    }
    return 0;
}

mrkdata_spec_t *
mrkdata_make_spec(mrkdata_tag_t tag)
{
    mrkdata_spec_t *spec, **e;

    if (tag < countof(builtin_specs)) {
        return &builtin_specs[tag];
    }

    if ((spec = malloc(sizeof(mrkdata_spec_t))) == NULL) {
        FAIL("malloc");
    }
    spec->tag = tag;
    if (tag == MRKDATA_STRUCT || tag == MRKDATA_SEQ) {
        if (list_init(&spec->fields, sizeof(mrkdata_spec_t *), 0,
                       NULL,
                       NULL) != 0) {
            FAIL("list_init");
        }
    }

    if ((e = array_incr(&specs)) == NULL) {
        FAIL("array_incr");
    }
    *e = spec;
    return spec;
}

void
mrkdata_spec_add_field(mrkdata_spec_t *spec, mrkdata_spec_t *field)
{
    mrkdata_spec_t **e;

    assert(spec->tag == MRKDATA_STRUCT || spec->tag == MRKDATA_SEQ);

    if ((e = list_incr(&spec->fields)) == NULL) {
        FAIL("list_incr");
    }
    *e = field;
}


/* datum */
static int
datum_init(mrkdata_datum_t *dat)
{
    dat->spec = NULL;
    dat->parent = NULL;
    dat->packsz = 0;
    return 0;
}

mrkdata_datum_t *
mrkdata_datum_from_spec(mrkdata_spec_t *spec, void *v, size_t sz)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = spec;

    res->packsz = EXPECT_SZ(spec->tag);

    if (spec->tag == MRKDATA_UINT8) {
        res->value.u8 = (uint8_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_INT8) {
        res->value.i8 = (int8_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_UINT16) {
        res->value.u16 = (uint16_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_INT16) {
        res->value.i16 = (int16_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_UINT32) {
        res->value.u32 = (uint32_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_INT32) {
        res->value.i32 = (int32_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_UINT64) {
        res->value.u64 = (uint64_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_INT64) {
        res->value.i64 = (int64_t)(uintptr_t)v;

    } else if (spec->tag == MRKDATA_STR8) {
        res->value.sz8 = (int8_t)sz;

        if (res->value.sz8 <=0 ) {
            goto ERR;
        }

        res->packsz += res->value.sz8;

        if ((res->data.str = malloc(res->value.sz8)) == NULL) {
            FAIL("malloc");
        }
        memcpy(res->data.str, v, res->value.sz8);

    } else if (spec->tag == MRKDATA_STR16) {
        res->value.sz16 = (int16_t)sz;

        if (res->value.sz16 <=0 ) {
            goto ERR;
        }

        res->packsz += res->value.sz16;

        if ((res->data.str = malloc(res->value.sz16)) == NULL) {
            FAIL("malloc");
        }
        memcpy(res->data.str, v, res->value.sz16);

    } else if (spec->tag == MRKDATA_STR32) {
        res->value.sz32 = (int32_t)sz;

        if (res->value.sz32 <=0 ) {
            goto ERR;
        }

        res->packsz += res->value.sz32;

        if ((res->data.str = malloc(res->value.sz32)) == NULL) {
            FAIL("malloc");
        }
        memcpy(res->data.str, v, res->value.sz32);

    } else if (spec->tag == MRKDATA_STR64) {
        res->value.sz64 = (int64_t)sz;

        if (res->value.sz64 <=0 ) {
            goto ERR;
        }

        res->packsz += res->value.sz64;

        if ((res->data.str = malloc(res->value.sz64)) == NULL) {
            FAIL("malloc");
        }
        memcpy(res->data.str, v, res->value.sz64);

    } else if (spec->tag == MRKDATA_STRUCT || spec->tag == MRKDATA_SEQ) {
        /* must be set in mrkdata_datum_adjust_packsz */
        res->value.sz64 = 0;

        if (list_init(&res->data.fields, sizeof(mrkdata_datum_t *), 0,
                      (list_initializer_t)null_pointer_initializer,
                      (list_finalizer_t)mrkdata_datum_destroy) != 0) {
            FAIL("list_init");
        }


    } else {
        assert(0);
    }

END:
    return res;

ERR:
    if (res != NULL) {
        free(res);
        res = NULL;
    }
    goto END;
}

static int
_mrkdata_datum_dump(mrkdata_datum_t *dat, int lvl)
{
    if (dat->spec->tag == MRKDATA_STRUCT || dat->spec->tag == MRKDATA_SEQ) {
        mrkdata_datum_t **o;
        list_iter_t it;

        LTRACE(lvl, "<datum tag=%s>", MRKDATA_TAG_STR(dat->spec->tag));

        for (o = list_first(&dat->data.fields, &it);
             o != NULL;
             o = list_next(&dat->data.fields, &it)) {
            _mrkdata_datum_dump(*o, lvl + 1);
        }
    } else {
        LTRACEN(lvl, "<datum tag=%s value=", MRKDATA_TAG_STR(dat->spec->tag));

        switch (dat->spec->tag) {
        case MRKDATA_UINT8:
            TRACEC("%02hhx>", dat->value.u8);
            break;

        case MRKDATA_INT8:
            TRACEC("%hhd>", dat->value.i8);
            break;

        case MRKDATA_UINT16:
            TRACEC("%04hx>", dat->value.u16);
            break;

        case MRKDATA_INT16:
            TRACEC("%hd>", dat->value.i16);
            break;

        case MRKDATA_UINT32:
            TRACEC("%08x>", dat->value.u32);
            break;

        case MRKDATA_INT32:
            TRACEC("%d>", dat->value.i32);
            break;

        case MRKDATA_UINT64:
            TRACEC("%016lx>", dat->value.u64);
            break;

        case MRKDATA_INT64:
            TRACEC("%ld>", dat->value.i64);
            break;

        case MRKDATA_DOUBLE:
            TRACEC("%lf>", dat->value.d);
            break;

        case MRKDATA_STR8:
            TRACEC("...>");
            D8(dat->data.str, dat->value.sz8);
            break;

        case MRKDATA_STR16:
            TRACEC("...>");
            D16(dat->data.str, dat->value.sz16);
            break;

        case MRKDATA_STR32:
            TRACEC("...>");
            D32(dat->data.str, dat->value.sz32);
            break;

        case MRKDATA_STR64:
            TRACEC("...>");
            D64(dat->data.str, dat->value.sz64);
            break;

        default:
            ;
        }
    }
    return 0;
}

int
mrkdata_datum_dump(mrkdata_datum_t *dat)
{
    return _mrkdata_datum_dump(dat, 0);
}

static int
datum_fini(mrkdata_datum_t *dat)
{
    if (dat->spec != NULL) {
        if (dat->spec->tag == MRKDATA_STR8 ||
            dat->spec->tag == MRKDATA_STR16 ||
            dat->spec->tag == MRKDATA_STR32 ||
            dat->spec->tag == MRKDATA_STR64) {

            if (dat->data.str != NULL) {
                free(dat->data.str);
                dat->data.str = NULL;
            }
        } else if (dat->spec->tag == MRKDATA_STRUCT || dat->spec->tag == MRKDATA_SEQ) {
            list_fini(&dat->data.fields);
        }

        dat->spec = NULL;
    }
    dat->parent = NULL;
    dat->packsz = 0;
    return 0;
}

int
mrkdata_datum_destroy(mrkdata_datum_t **dat)
{
    if (*dat != NULL) {
        datum_fini(*dat);
        free(*dat);
        *dat = NULL;
    }
    return 0;
}

void
mrkdata_datum_add_field(mrkdata_datum_t *dat, mrkdata_datum_t *field)
{
    mrkdata_datum_t **pdat;

    assert(dat->spec->tag == MRKDATA_STRUCT || dat->spec->tag == MRKDATA_SEQ);

    if ((pdat = list_incr(&dat->data.fields)) == NULL) {
        FAIL("list_incr");
    }

    *pdat = field;
    mrkdata_datum_adjust_packsz(dat, field->packsz);
}

mrkdata_datum_t *
mrkdata_datum_get_field(mrkdata_datum_t *dat, unsigned idx)
{
    mrkdata_datum_t **pfield;

    assert(dat->spec->tag == MRKDATA_STRUCT || dat->spec->tag == MRKDATA_SEQ);

    if ((pfield = list_get(&dat->data.fields, idx)) == NULL) {
        return NULL;
    }

    return *pfield;
}

mrkdata_datum_t *
mrkdata_datum_make_u8(uint8_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_UINT8];
    res->packsz = EXPECT_SZ(MRKDATA_UINT8);
    res->value.u8 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_i8(int8_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_INT8];
    res->packsz = EXPECT_SZ(MRKDATA_INT8);
    res->value.i8 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_u16(uint16_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_UINT16];
    res->packsz = EXPECT_SZ(MRKDATA_UINT16);
    res->value.u16 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_i16(int16_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_INT16];
    res->packsz = EXPECT_SZ(MRKDATA_INT16);
    res->value.i16 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_u32(uint32_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_UINT32];
    res->packsz = EXPECT_SZ(MRKDATA_UINT32);
    res->value.u32 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_i32(int32_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_INT32];
    res->packsz = EXPECT_SZ(MRKDATA_INT32);
    res->value.i32 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_u64(uint64_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_UINT64];
    res->packsz = EXPECT_SZ(MRKDATA_UINT64);
    res->value.u64 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_i64(int64_t v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_INT64];
    res->packsz = EXPECT_SZ(MRKDATA_INT64);
    res->value.i64 = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_double(double v)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_DOUBLE];
    res->packsz = EXPECT_SZ(MRKDATA_DOUBLE);
    res->value.d = v;
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_str8(char *v, int8_t sz)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_STR8];
    res->packsz = EXPECT_SZ(MRKDATA_STR8) + sz;
    res->value.sz8 = sz;
    if ((res->data.str = malloc(sz)) == NULL) {
        FAIL("malloc");
    }
    if (v != NULL) {
        memcpy(res->data.str, v, sz);
    } else {
        memset(res->data.str, '\0', sz);
    }
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_str16(char *v, int16_t sz)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_STR16];
    res->packsz = EXPECT_SZ(MRKDATA_STR16) + sz;
    res->value.sz16 = sz;
    if ((res->data.str = malloc(sz)) == NULL) {
        FAIL("malloc");
    }
    if (v != NULL) {
        memcpy(res->data.str, v, sz);
    } else {
        memset(res->data.str, '\0', sz);
    }
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_str32(char *v, int32_t sz)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_STR32];
    res->packsz = EXPECT_SZ(MRKDATA_STR32) + sz;
    res->value.sz32 = sz;
    if ((res->data.str = malloc(sz)) == NULL) {
        FAIL("malloc");
    }
    if (v != NULL) {
        memcpy(res->data.str, v, sz);
    } else {
        memset(res->data.str, '\0', sz);
    }
    return res;
}

mrkdata_datum_t *
mrkdata_datum_make_str64(char *v, int64_t sz)
{
    mrkdata_datum_t *res;

    if ((res = malloc(sizeof(mrkdata_datum_t))) == NULL) {
        FAIL("malloc");
    }
    datum_init(res);
    res->spec = &builtin_specs[MRKDATA_STR64];
    res->packsz = EXPECT_SZ(MRKDATA_STR64) + sz;
    res->value.sz64 = sz;
    if ((res->data.str = malloc(sz)) == NULL) {
        FAIL("malloc");
    }
    if (v != NULL) {
        memcpy(res->data.str, v, sz);
    } else {
        memset(res->data.str, '\0', sz);
    }
    return res;
}

/* module */
void
mrkdata_init(void)
{
    size_t i;

    if (mflags & MRKDATA_MFLAG_INITIALIZED) {
        return;
    }

    MEMDEBUG_REGISTER(mrkdata);

    for (i = 0; i < countof(builtin_specs); ++i) {
        builtin_specs[i].tag = i;
    }

    if (array_init(&specs, sizeof(mrkdata_spec_t *), 0,
                   (array_initializer_t)null_pointer_initializer,
                   (array_finalizer_t)mrkdata_spec_destroy) != 0) {
        FAIL("array_init");
    }
    mflags |= MRKDATA_MFLAG_INITIALIZED;
}

void
mrkdata_fini(void)
{
    if (!(mflags & MRKDATA_MFLAG_INITIALIZED)) {
        return;
    }
    array_fini(&specs);
    mflags &= ~MRKDATA_MFLAG_INITIALIZED;
}

