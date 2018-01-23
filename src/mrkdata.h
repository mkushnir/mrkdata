#ifndef MRKDATA_H
#define MRKDATA_H

#include <sys/types.h>

#include "mrkcommon/array.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *mrkdata_diag_str(int);

/*
 * Sync with tag_sz[]
 */
typedef enum _mrkdata_tag {
    MRKDATA_UINT8,
    MRKDATA_INT8,
    MRKDATA_UINT16,
    MRKDATA_INT16,
    MRKDATA_UINT32,
    MRKDATA_INT32,
    MRKDATA_UINT64,
    MRKDATA_INT64,
    MRKDATA_DOUBLE,
    MRKDATA_STR8,
    MRKDATA_STR16,
    MRKDATA_STR32,
    MRKDATA_STR64,
    /*
     * End of built-in specs.
     *
     * Below are tags for user-defined specs.
     */
    MRKDATA_STRUCT,
    MRKDATA_SEQ,
    MRKDATA_DICT,
    MRKDATA_FUNC,
} mrkdata_tag_t;

#define MRKDATA_BUILTIN_TAG_END (MRKDATA_STR64 + 1)
#define MRKDATA_TAG_END (MRKDATA_FUNC + 1)

#define MRKDATA_TAG_CUSTOM(tag) \
    ((tag) == MRKDATA_STRUCT || \
     (tag) == MRKDATA_SEQ || \
     (tag) == MRKDATA_DICT || \
     (tag) == MRKDATA_FUNC)

#define MRKDATA_TAG_STR(tag) \
    (tag == MRKDATA_UINT8 ? "UINT8" : \
     tag == MRKDATA_INT8 ? "INT8" : \
     tag == MRKDATA_UINT16 ? "UINT16" : \
     tag == MRKDATA_INT16 ? "INT16" : \
     tag == MRKDATA_UINT32 ? "UINT32" : \
     tag == MRKDATA_INT32 ? "INT32" : \
     tag == MRKDATA_UINT64 ? "UINT64" : \
     tag == MRKDATA_INT64 ? "INT64" : \
     tag == MRKDATA_DOUBLE ? "DOUBLE" : \
     tag == MRKDATA_STR8 ? "STR8" : \
     tag == MRKDATA_STR16 ? "STR16" : \
     tag == MRKDATA_STR32 ? "STR32" : \
     tag == MRKDATA_STR64 ? "STR64" : \
     tag == MRKDATA_STRUCT ? "STRUCT" : \
     tag == MRKDATA_SEQ ? "SEQ" : \
     tag == MRKDATA_DICT ? "DICT" : \
     tag == MRKDATA_FUNC ? "FUNC" : \
     "<unknown>")

struct _mrkdata_datum;

typedef struct _mrkdata_spec {
    /*
     * "custom" name, for structure members and function formal
     * parameters
     */
    char *name;
    mnarray_t fields;
    mrkdata_tag_t tag;
} mrkdata_spec_t;

typedef struct _mrkdata_datum {
    const mrkdata_spec_t *spec;
    union {
        uint8_t u8;
        int8_t i8;
        uint16_t u16;
        int16_t i16;
        uint32_t u32;
        int32_t i32;
        uint64_t u64;
        int64_t i64;
        int8_t sz8;
        int16_t sz16;
        int32_t sz32;
        int64_t sz64;
        double d;
    } value;
    union {
        char *str;
        mnarray_t fields;
    } data;
    ssize_t packsz;
    struct _mrkdata_datum *parent;
} mrkdata_datum_t;


void mrkdata_init(void);
void mrkdata_fini(void);
ssize_t mrkdata_parse_buf(const unsigned char *buf,
                          ssize_t sz,
                          int (*cb)(const unsigned char *,
                                    mrkdata_tag_t,
                                    ssize_t,
                                    void *),
                          void *udata);
ssize_t mrkdata_unpack_buf(const mrkdata_spec_t *,
                           const unsigned char *,
                           ssize_t,
                           mrkdata_datum_t **);
ssize_t mrkdata_pack_datum(const mrkdata_datum_t *,
                           unsigned char *,
                           ssize_t);
mrkdata_spec_t *mrkdata_make_spec(mrkdata_tag_t);
void mrkdata_spec_set_name(mrkdata_spec_t *, const char *);
void mrkdata_spec_add_field(mrkdata_spec_t *, mrkdata_spec_t *);
int mrkdata_spec_destroy(mrkdata_spec_t **);
int mrkdata_spec_dump(mrkdata_spec_t *);
int mrkdata_datum_destroy(mrkdata_datum_t **);
int mrkdata_datum_dump(mrkdata_datum_t *);
void mrkdata_datum_add_field(mrkdata_datum_t *, mrkdata_datum_t *);
mrkdata_datum_t *mrkdata_datum_get_field(mrkdata_datum_t *, unsigned);
mrkdata_datum_t *mrkdata_datum_from_spec(mrkdata_spec_t *, void *, size_t);
mrkdata_datum_t *mrkdata_datum_make_u8(uint8_t);
mrkdata_datum_t *mrkdata_datum_make_i8(int8_t);
mrkdata_datum_t *mrkdata_datum_make_u16(uint16_t);
mrkdata_datum_t *mrkdata_datum_make_i16(int16_t);
mrkdata_datum_t *mrkdata_datum_make_u32(uint32_t);
mrkdata_datum_t *mrkdata_datum_make_i32(int32_t);
mrkdata_datum_t *mrkdata_datum_make_u64(uint64_t);
mrkdata_datum_t *mrkdata_datum_make_i64(int64_t);
mrkdata_datum_t *mrkdata_datum_make_double(double);
mrkdata_datum_t *mrkdata_datum_make_str8(char *, int8_t);
mrkdata_datum_t *mrkdata_datum_make_str16(char *, int16_t);
mrkdata_datum_t *mrkdata_datum_make_str32(char *, int32_t);
mrkdata_datum_t *mrkdata_datum_make_str64(char *, int64_t);

#ifdef __cplusplus
}
#endif

#endif
