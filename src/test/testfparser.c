#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include "mrkcommon/dumpm.h"
#include "mrkcommon/util.h"

#include "unittest.h"
#include "fparser.h"

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

}

static void
test1(void)
{
    int fd;
    int res;

    if ((fd = open("data-02", O_RDONLY)) < 0) {
        assert(0);
    }

    res = fparser_parse(fd);
    TRACE("res=%d", res);
    close(fd);
}

int
main(void)
{
    mrkdata_init();
    test0();
    test1();
    mrkdata_fini();
    return 0;
}
