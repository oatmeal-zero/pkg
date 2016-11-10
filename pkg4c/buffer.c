#include "define.h"

char* mbnew(const char *init)
{
    return sdsnew(init);
}

char* mbnewlen(const void *init, size_t initlen)
{
    return sdsnewlen(init, initlen);
}

char* mbempty()
{
    return sdsempty();
}

void mbfree(char* buf)
{
    sdsfree(buf);
}

size_t mbsize(char* buf)
{
    return sdslen(buf);
}

const char* mbcstr(char* buf)
{
    return buf;
}

