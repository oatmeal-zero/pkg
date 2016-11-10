#include "mypkg.h"
#include "pkgapi.h"

CMyBuffer::CMyBuffer()
{
    buf = mbempty();
}

CMyBuffer::CMyBuffer(const char *init)
{
    buf = mbnew(init);
}

CMyBuffer::CMyBuffer(const char *init, size_t initlen)
{
    buf = mbnewlen(init, initlen);
}

CMyBuffer& CMyBuffer::operator=(char *buf)
{
    mbfree(this->buf);
    this->buf = buf;
    return *this;
}

CMyBuffer::~CMyBuffer()
{
    mbfree(buf);
}

const char* CMyBuffer::c_str() const
{
    return mbcstr(buf);
}

size_t CMyBuffer::size() const
{
    return mbsize(buf);
}
