#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mrkcommon/dumpm.h"
#include "mrkcommon/util.h"
#include "mrkcommon/memdebug.h"

MEMDEBUG_DECLARE(testfoo);

#include "unittest.h"
#include "mrkdata.h"

UNUSED static void
test_pack_uint8(void)
{
    mrkdata_datum_t *dat = NULL;
    mrkdata_datum_t *rdat = NULL;
    unsigned char *buf;
    ssize_t nwritten;
    ssize_t nread;

    if ((dat = mrkdata_datum_from_spec(mrkdata_make_spec(MRKDATA_UINT8),
                                       (void *)123, 0)) == NULL) {
        assert(0);
    }

    if ((buf = malloc(dat->packsz)) == NULL) {
        assert(0);
    }

    nwritten = mrkdata_pack_datum(dat, buf, (ssize_t)dat->packsz);
    TRACE("nwritten=%ld", nwritten);
    D8(buf, dat->packsz);

    nread = mrkdata_unpack_buf(dat->spec, buf, (ssize_t)dat->packsz, &rdat);
    TRACE("nread=%ld", nread);

    mrkdata_datum_dump(dat);

    mrkdata_datum_dump(rdat);

    mrkdata_datum_destroy(&dat);
    mrkdata_datum_destroy(&rdat);
    free(buf);
}

UNUSED static void
test_pack_str8(void)
{
    mrkdata_datum_t *dat = NULL;
    mrkdata_datum_t *rdat = NULL;
    unsigned char *buf;
    ssize_t nwritten;
    ssize_t nread;
    const char *s = "This is the test";

    if ((dat = mrkdata_datum_from_spec(mrkdata_make_spec(MRKDATA_STR8),
                                       (void *)s, strlen(s) + 1)) == NULL) {
        assert(0);
    }

    if ((buf = malloc(dat->packsz)) == NULL) {
        assert(0);
    }

    nwritten = mrkdata_pack_datum(dat, buf, (ssize_t)dat->packsz);
    TRACE("nwritten=%ld", nwritten);
    D8(buf, dat->packsz);

    nread = mrkdata_unpack_buf(dat->spec, buf, (ssize_t)dat->packsz, &rdat);
    TRACE("nread=%ld", nread);

    mrkdata_datum_dump(dat);

    mrkdata_datum_dump(rdat);

    mrkdata_datum_destroy(&dat);
    mrkdata_datum_destroy(&rdat);
    free(buf);
}

UNUSED static void
test_pack_seq(void)
{
    mrkdata_spec_t *strspec, *seqspec;
    mrkdata_datum_t *dat = NULL;
    ssize_t nwritten;
    mrkdata_datum_t *rdat = NULL;
    ssize_t nread;
    unsigned char *buf;
    const char *s1 = "This is the test one";
    const char *s2 = "This is the test two 22";
    const char *s3 = "This is the test three 33333";

    seqspec = mrkdata_make_spec(MRKDATA_SEQ);
    strspec = mrkdata_make_spec(MRKDATA_STR8);
    mrkdata_spec_add_field(seqspec, strspec);

    if ((dat = mrkdata_datum_from_spec(seqspec, NULL, 0)) == NULL) {
        assert(0);
    }

    mrkdata_datum_add_field(dat, mrkdata_datum_from_spec(strspec, (void *)s1, strlen(s1) + 1));
    mrkdata_datum_add_field(dat, mrkdata_datum_from_spec(strspec, (void *)s2, strlen(s2) + 1));
    mrkdata_datum_add_field(dat, mrkdata_datum_from_spec(strspec, (void *)s3, strlen(s3) + 1));

    TRACE("dat->packsz=%ld", dat->packsz);

    if ((buf = malloc(dat->packsz)) == NULL) {
        assert(0);
    }

    nwritten = mrkdata_pack_datum(dat, buf, (ssize_t)dat->packsz);
    TRACE("nwritten=%ld", nwritten);
    D8(buf, dat->packsz);

    nread = mrkdata_unpack_buf(dat->spec, buf, (ssize_t)dat->packsz, &rdat);
    TRACE("nread=%ld", nread);

    TRACE("dat:");
    mrkdata_datum_dump(dat);

    TRACE("rdat:");
    mrkdata_datum_dump(rdat);

    mrkdata_datum_destroy(&rdat);

    mrkdata_datum_destroy(&dat);

    free(buf);
}

UNUSED static void
test_pack_struct(void)
{
    mrkdata_spec_t *structspec, *seqspec, *strspec, *i8spec, *ui64spec;
    mrkdata_datum_t *dat = NULL, *seqdat = NULL;
    ssize_t nwritten;
    mrkdata_datum_t *rdat = NULL;
    ssize_t nread;
    unsigned char *buf;
    const char *s1 = "This is the test one";
    const char *s2 = "This is the test two 22";
    const char *s3 = "This is the test three 33333";

    strspec = mrkdata_make_spec(MRKDATA_STR8);
    i8spec = mrkdata_make_spec(MRKDATA_INT8);
    ui64spec = mrkdata_make_spec(MRKDATA_UINT64);

    seqspec = mrkdata_make_spec(MRKDATA_SEQ);
    mrkdata_spec_add_field(seqspec, strspec);

    structspec = mrkdata_make_spec(MRKDATA_STRUCT);
    mrkdata_spec_add_field(structspec, seqspec);
    mrkdata_spec_add_field(structspec, i8spec);
    mrkdata_spec_add_field(structspec, ui64spec);

    if ((seqdat = mrkdata_datum_from_spec(seqspec, NULL, 0)) == NULL) {
        assert(0);
    }

    mrkdata_datum_add_field(seqdat, mrkdata_datum_from_spec(strspec, (void *)s1, strlen(s1) + 1));
    mrkdata_datum_add_field(seqdat, mrkdata_datum_from_spec(strspec, (void *)s2, strlen(s2) + 1));
    mrkdata_datum_add_field(seqdat, mrkdata_datum_from_spec(strspec, (void *)s3, strlen(s3) + 1));

    if ((dat = mrkdata_datum_from_spec(structspec, NULL, 0)) == NULL) {
        assert(0);
    }

    mrkdata_datum_add_field(dat, seqdat);
    mrkdata_datum_add_field(dat, mrkdata_datum_from_spec(i8spec, (void *)123, 0));
    mrkdata_datum_add_field(dat, mrkdata_datum_from_spec(ui64spec, (void *)0x123456789, 0));

    TRACE("dat->packsz=%ld", dat->packsz);

    if ((buf = malloc(dat->packsz)) == NULL) {
        assert(0);
    }

    nwritten = mrkdata_pack_datum(dat, buf, (ssize_t)dat->packsz);
    TRACE("nwritten=%ld", nwritten);
    D8(buf, dat->packsz);

    nread = mrkdata_unpack_buf(dat->spec, buf, (ssize_t)dat->packsz, &rdat);
    TRACE("nread=%ld", nread);

    TRACE("dat:");
    mrkdata_datum_dump(dat);

    TRACE("rdat:");
    mrkdata_datum_dump(rdat);

    mrkdata_datum_destroy(&rdat);

    mrkdata_datum_destroy(&dat);

    free(buf);
}

UNUSED static void
test_unpack_uint8(void)
{
    int fd;
    struct stat sb;
    unsigned char *buf, *pbuf;
    ssize_t sz;
    ssize_t nread;
    mrkdata_spec_t *spec = NULL;
    mrkdata_datum_t *dat = NULL;

    fd = open("data-00", O_RDONLY);
    assert(fd >= 0);

    if (fstat(fd, &sb) != 0) {
        perror("fstat");
        assert(0);
    }

    sz = sb.st_size;

    if ((buf = malloc(sz)) == NULL) {
        perror("malloc");
        assert(0);
    }

    pbuf = buf;

    if (read(fd, buf, sz) != sz) {
        perror("read");
        assert(0);
    }

    spec = mrkdata_make_spec(MRKDATA_UINT8);

    if ((nread = mrkdata_unpack_buf(spec, pbuf, sz, &dat)) == 0) {
        assert(0);
    }

    //TRACE("nread=%ld", nread);
    sz -= nread;
    pbuf += nread;
    TRACE("u8=%d size=%ld", dat->value.u8, dat->packsz);

    mrkdata_datum_destroy(&dat);
    mrkdata_spec_destroy(&spec);

    spec = mrkdata_make_spec(MRKDATA_INT8);

    //TRACE("sz=%ld", sz);
    if ((nread = mrkdata_unpack_buf(spec, pbuf, sz, &dat)) == 0) {
        assert(0);
    }
    sz -= nread;
    pbuf += nread;
    TRACE("i8=%d size=%ld", dat->value.i8, dat->packsz);

    mrkdata_datum_destroy(&dat);
    mrkdata_spec_destroy(&spec);

    TRACE("sz=%ld", sz);
    free(buf);
    close(fd);
}

UNUSED static void
test_unpack_str8(void)
{
    int fd;
    struct stat sb;
    unsigned char *buf, *pbuf;
    ssize_t sz;
    ssize_t nread;
    mrkdata_spec_t *spec = NULL;
    mrkdata_datum_t *dat = NULL;

    fd = open("data-01", O_RDONLY);
    assert(fd >= 0);

    if (fstat(fd, &sb) != 0) {
        perror("fstat");
        assert(0);
    }

    sz = sb.st_size;

    if ((buf = malloc(sz)) == NULL) {
        perror("malloc");
        assert(0);
    }

    pbuf = buf;

    if (read(fd, buf, sz) != (ssize_t)sz) {
        perror("read");
        assert(0);
    }

    spec = mrkdata_make_spec(MRKDATA_STR8);

    if ((nread = mrkdata_unpack_buf(spec, pbuf, sz, &dat)) == 0) {
        assert(0);
    }
    sz -= nread;
    pbuf += nread;

    TRACE("sz8=%d size=%ld", dat->value.sz8, dat->packsz);

    D8(dat->data.str, dat->value.sz8);

    mrkdata_datum_destroy(&dat);
    mrkdata_spec_destroy(&spec);

    TRACE("sz=%ld", sz);
    free(buf);

    close(fd);
}

UNUSED static void
test_unpack_struct1(void)
{
    int fd;
    struct stat sb;
    unsigned char *buf, *pbuf;
    ssize_t sz;
    ssize_t nread;
    mrkdata_spec_t *spec = NULL;
    mrkdata_datum_t *dat = NULL;

    fd = open("data-03", O_RDONLY);
    assert(fd >= 0);

    if (fstat(fd, &sb) != 0) {
        perror("fstat");
        assert(0);
    }

    sz = sb.st_size;

    if ((buf = malloc(sz)) == NULL) {
        perror("malloc");
        assert(0);
    }

    pbuf = buf;

    if (read(fd, buf, sz) != (ssize_t)sz) {
        perror("read");
        assert(0);
    }

    spec = mrkdata_make_spec(MRKDATA_STRUCT);
    mrkdata_spec_add_field(spec, mrkdata_make_spec(MRKDATA_INT8));
    mrkdata_spec_add_field(spec, mrkdata_make_spec(MRKDATA_UINT8));
    mrkdata_spec_add_field(spec, mrkdata_make_spec(MRKDATA_STR8));

    if ((nread = mrkdata_unpack_buf(spec, pbuf, sz, &dat)) == 0) {
        assert(0);
    }

    TRACE("nread=%ld", nread);

    sz -= nread;
    pbuf += nread;

    mrkdata_datum_dump(dat);

    mrkdata_datum_destroy(&dat);
    //mrkdata_spec_destroy(&spec);

    TRACE("sz=%ld", sz);
    free(buf);

    close(fd);
}

static void
test0(void)
{
    struct {
        long rnd;
        int in;
        int expected;
    } data[] = {
        {0, 0, 0},
    };

    UNITTEST_PROLOG_RAND;

    FOREACHDATA {
        TRACE("in=%d expected=%d", CDATA.in, CDATA.expected);
        assert(CDATA.in == CDATA.expected);
    }

    //test_pack_uint8();
    //test_pack_str8();
    //test_pack_seq();
    test_pack_struct();

    //test_unpack_uint8();
    //test_unpack_str8();
    //test_unpack_struct1();
}

int
main(void)
{
    MEMDEBUG_REGISTER(testfoo);

    mrkdata_init();
    test0();
    mrkdata_fini();

    memdebug_print_stats();

    return 0;
}
