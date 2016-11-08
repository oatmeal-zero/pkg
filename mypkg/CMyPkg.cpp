
//#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mypkg.h"
#include "util.h"
#include "pkgapi.h"

struct myval_t;
typedef CMyPkg::Iterator Iterator;
typedef CMyPkg::Proxy    Proxy;

unsigned short CRC_table[256] =
{
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
};

unsigned short do_crc_table(const unsigned char *data, int length)
{
	unsigned short fcs = 0x0000; // 初始化

	while(length > 0)
	{
		fcs = fcs <<8;
		fcs = fcs^CRC_table[fcs>>8^*data];
		length--;
		data++;
	} 
	return fcs; 
}

CMyPkg::CMyPkg()
{
    pkg = mpnew();
}

CMyPkg::CMyPkg(mypkg *pkg)
{
    this->pkg = pkg;
}

CMyPkg::CMyPkg(const CMyPkg& rhs)
{
    pkg = mpdup(rhs.pkg);
}

CMyPkg& CMyPkg::operator=(const CMyPkg& rhs)
{
	if (&rhs == this) return *this;
    mpfree(pkg);
    pkg = mpdup(rhs.pkg);
	return *this;
}

CMyPkg::~CMyPkg()
{
    mpfree(pkg);
}

CMyPkg CMyPkg::ParseJsonFromString(const char *text)
{
    return CMyPkg(mpparse_json(text));
}

CMyPkg CMyPkg::ParseJsonFromFile(const char *file)
{
    return CMyPkg(mpparse_json_file(file));
}

void CMyPkg::print_json() const
{
    mpprint_json(pkg);
}

void CMyPkg::print_json_unformat() const
{
    mpprint_json_unformat(pkg);
}

void CMyPkg::clear()
{
    mpclear(pkg);
}

void CMyPkg::print(const char *msg) const
{
    mpprint(pkg, msg);
}

const Iterator CMyPkg::begin() const
{
    return Iterator(this);
}

const Iterator& CMyPkg::end() const
{
    static Iterator IterEnd;
    return IterEnd;
}

int CMyPkg::addVal(const char *key, char val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, bool val)
{
    return mpaddboolean(pkg, key, val);
}

int CMyPkg::addVal(const char *key, short val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, int val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, long val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, u_short val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, u_int val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, u_long val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, integer_t val)
{
    return mpaddinteger(pkg, key, val);
}

int CMyPkg::addVal(const char *key, float val)
{
    return mpaddnumber(pkg, key, val);
}

int CMyPkg::addVal(const char *key, number_t val)
{
    return mpaddnumber(pkg, key, val);
}

int CMyPkg::addVal(const char *key, const char *val)
{
    return mpaddstring(pkg, key, val, strlen(val));
}

int CMyPkg::addVal(const char *key, const char *val, int len)
{
    return mpaddstring(pkg, key, val, len);
}

int CMyPkg::addVal(const char *key, string& val)
{
    return mpaddstring(pkg, key, val.c_str(), val.size());
}

int CMyPkg::addVal(const char *key, const CMyPkg& val)
{
    return mpaddobject(pkg, key, val.pkg);
}

int CMyPkg::addVal(const char *key, const CMyPkg* val)
{
    return mpaddobject(pkg, key, val->pkg);
}

int CMyPkg::addVal(const char *key, const Proxy& val)
{
    int ret = -1;
    switch (val.type()) {
        case MP_T_NIL:
            ret = delField(key);
            break;
        case MP_T_BOOLEAN:
            ret = addVal(key, (bool)mpvalint(val.val));
            break;
        case MP_T_INTEGER:
            ret = addVal(key, mpvalint(val.val));
            break;
        case MP_T_NUMBER:
            ret = addVal(key, mpvalnum(val.val));
            break;
        case MP_T_STRING:
            {
            size_t len;
            const char *str = mpvalstr(val.val, &len);
            ret = addVal(key, str, len);
            break;
            }
        case MP_T_OBJECT:
            ret = mpaddobject(pkg, key, mpvalobj(val.val));
            break;
    }
    return ret;
}

Proxy CMyPkg::getVal(const char *key) const
{
    return Proxy(mpgetval(pkg, key));
}

Proxy CMyPkg::getVal(int index) const
{
    return Proxy(mpgetaval(pkg, index));
}

Proxy CMyPkg::operator[](const char* key) 
{
	myval_t *val = mpgetval(pkg, key);
    if (val == NULL) val = mpaddnil(pkg, key);
	return Proxy(val);
}

Proxy CMyPkg::operator[](int index) 
{
	myval_t *val = mpgetaval(pkg, index);
    if (val == NULL) val = mpassignnil(pkg, index);
	return Proxy(val);
}

const char* CMyPkg::getString(const char *key, size_t& len) const
{
    return mpgetlstring(pkg, key, &len);
}

int CMyPkg::getArraySize() const
{
    return mpgetarraylen(pkg);
}

int CMyPkg::delField(const char *key)
{
	return mpdelnode(pkg, key);
}

bool CMyPkg::exist(const char* key)
{
	return (mpexist(pkg, key) == 1);
}

int CMyPkg::appendVal(char val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(bool val)
{
    return mpappendbool(pkg, val);
}

int CMyPkg::appendVal(short val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(int val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(long val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(u_short val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(u_int val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(u_long val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(integer_t val)
{
    return mpappendint(pkg, val);
}

int CMyPkg::appendVal(float val)
{
    return mpappendnum(pkg, val);
}

int CMyPkg::appendVal(number_t val)
{
    return mpappendnum(pkg, val);
}

int CMyPkg::appendVal(const char *val)
{
    return mpappendstr(pkg, val, strlen(val));
}

int CMyPkg::appendVal(const char *val, int len)
{
    return mpappendstr(pkg, val, len);
}

int CMyPkg::appendVal(string& val)
{
    return mpappendstr(pkg, val.c_str(), val.size());
}

int CMyPkg::appendVal(const CMyPkg& val)
{
    return mpappendobj(pkg, val.pkg);
}

int CMyPkg::appendVal(const CMyPkg* val)
{
    return mpappendobj(pkg, val->pkg);
}

int CMyPkg::appendVal(const Proxy& val)
{
    int ret = -1;
    switch (val.type()) {
        case MP_T_BOOLEAN:
            ret = appendVal((bool)mpvalint(val.val));
            break;
        case MP_T_INTEGER:
            ret = appendVal(mpvalint(val.val));
            break;
        case MP_T_NUMBER:
            ret = appendVal(mpvalnum(val.val));
            break;
        case MP_T_STRING:
            {
            size_t len;
            const char *str = mpvalstr(val.val, &len);
            ret = appendVal(str, len);
            break;
            }
        case MP_T_OBJECT:
            ret = mpappendobj(pkg, mpvalobj(val.val));
            break;
    }
    return ret;
}

int CMyPkg::CheckName(const char *name)
{
    if (name == NULL || name[0] == 0) {
		return INVALID_FLDNAME;
    }

	for (int i=0; name[i] != 0; i++) {
        char ch = name[i];
        if (!IsLetter(ch) && !IsNumber(ch) && !IsLegal(ch)) {
			return INVALID_FLDNAME;
		}
	}

    return 0;
}

Iterator::Iterator(const CMyPkg *pkg)
{
    iter = mpiternew(pkg->pkg);
}
Iterator::Iterator()
{
    iter = mpiterend();
}

Iterator::~Iterator()
{
    mpiterfree(iter);
}

bool Iterator::operator==(const Iterator& rhs)
{
    return (mpitercmp(this->iter, rhs.iter));
}

bool Iterator::operator!=(const Iterator& rhs)
{
    return !(operator==(rhs));
}

const Iterator& Iterator::operator++()
{
    mpiternext(iter);
    return *this;
}

const char* Iterator::key() const
{
    return mpiterkey(iter);
}

int Iterator::type() const
{
    return mpitertype(iter);
}

Proxy Iterator::getVal() const
{
    return Proxy(mpiterval(iter, NULL));
}

const char* Iterator::getString(size_t& len) const
{
    return Proxy(mpiterval(iter, &len));
}

Proxy::Proxy(myval_t *val)
{
    this->val = val;
}

Proxy::~Proxy()
{
}

int Proxy::type() const
{
    return mpvaltype(val);
}

Proxy::operator const char*() const
{
    size_t len;
    return mpvalstr(val, &len);
}

Proxy::operator string() const
{
    size_t len;
    const char *str = mpvalstr(val, &len);
    return string(str, len);
}

Proxy::operator bool() const
{
    return (mpvalint(val)!=0);
}

Proxy::operator short() const
{
    return mpvalint(val);
}

Proxy::operator int() const
{
    return mpvalint(val);
}

Proxy::operator long() const
{
    return mpvalint(val);
}

Proxy::operator u_short() const
{
    return mpvalint(val);
}

Proxy::operator u_int() const
{
    return mpvalint(val);
}

Proxy::operator u_long() const
{
    return mpvalint(val);
}

Proxy::operator integer_t() const
{
    return mpvalint(val);
}

Proxy::operator float() const
{
    return mpvalnum(val);
}

Proxy::operator number_t() const
{
    return mpvalnum(val);
}

Proxy::operator CMyPkg() const
{
    return CMyPkg(mpvalobj(val));
}

Proxy& Proxy::operator=(char val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(bool val)
{
    mpsetvalbool(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(short val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(int val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(long val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(u_short val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(u_int val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(u_long val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(integer_t val)
{
    mpsetvalint(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(float val)
{
    mpsetvalnum(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(number_t val)
{
    mpsetvalnum(this->val, val);
    return *this;
}

Proxy& Proxy::operator=(const char *val)
{
    mpsetvalstr(this->val, val, strlen(val));
    return *this;
}

Proxy& Proxy::operator=(string& val)
{
    mpsetvalstr(this->val, val.c_str(), val.size());
    return *this;
}

Proxy& Proxy::operator=(const CMyPkg& val)
{
    mpsetvalobj(this->val, val.pkg);
    return *this;
}

Proxy& Proxy::operator=(const CMyPkg* val)
{
    mpsetvalobj(this->val, val->pkg);
    return *this;
}

Proxy& Proxy::operator=(const Proxy& val)
{
    switch (val.type()) {
        case MP_T_BOOLEAN:
            mpsetvalbool(this->val, mpvalint(val.val));
            break;
        case MP_T_INTEGER:
            mpsetvalint(this->val, mpvalint(val.val));
            break;
        case MP_T_NUMBER:
            mpsetvalnum(this->val, mpvalnum(val.val));
            break;
        case MP_T_STRING:
            {
            size_t len;
            const char *str = mpvalstr(val.val, &len);
            mpsetvalstr(this->val, str, len);
            break;
            }
        case MP_T_OBJECT:
            mpsetvalobj(this->val, mpvalobj(val.val));
            break;
    }
    return *this;
}

Proxy Proxy::operator[](const char* key) 
{
    mypkg *pkg = mpvalobj(val);
	myval_t *val = mpgetval(pkg, key);
    if (val == NULL) val = mpaddnil(pkg, key);
	return Proxy(val);
}

Proxy Proxy::operator[](int index) 
{
    mypkg *pkg = mpvalobj(val);
	myval_t *val = mpgetaval(pkg, index);
    if (val == NULL) val = mpassignnil(pkg, index);
	return Proxy(val);
}

void Proxy::print(const char *msg) const
{
    mypkg *pkg = mpvalobj(val);
    mpprint(pkg, msg);
}

