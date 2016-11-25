#ifndef INCL_MYPKG
#define INCL_MYPKG

#ifndef NULL
#define NULL    0
#endif

#include <vector>
#include <string>

using namespace std;

typedef unsigned int	u_int;
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef long long int   integer_t;
typedef double          number_t;

// mypkg 字段变量类型定义
#define MP_T_NIL            0               // 空
#define MP_T_BOOLEAN 		1				// 布尔值
#define MP_T_INTEGER 		2				// 整型
#define MP_T_NUMBER 	    3				// 浮点数
#define MP_T_STRING 	    4				// 字符串
#define MP_T_OBJECT         5               // 数组类型

#define MEM_ERROR			1			// 系统内存不足
#define	OUTBUF_TOO_SMALL	2			// 传出数据缓冲长度不足
#define INVALID_FLDNAME	    3			// 字段名非法
#define FLD_NOT_FOUND 	    4			// 找不到字段
#define PKG_ERROR			5 			// 数据包内容不正确
#define CRC_ERROR			6 			// CRC校验包错误

struct mypkg;
struct myiter;
struct myval_t;

class CMyBuffer
{
    friend class CMyPkg;
public:
    CMyBuffer();
    CMyBuffer(const char *init);
    CMyBuffer(const char *init, size_t initlen);
    ~CMyBuffer();
public:
    const char* c_str() const;
    size_t size() const;
private:
    CMyBuffer& operator=(char *buf);
    char *buf;
};

class CMyPkg
{
public:
    class Proxy
    {
        friend class CMyPkg;
        myval_t    *val;
    public:
        Proxy(myval_t *val);
        ~Proxy();
    public:
        Proxy& operator=(char val);
        Proxy& operator=(bool val);
        Proxy& operator=(short val);
        Proxy& operator=(int val);
        Proxy& operator=(long val);
        Proxy& operator=(u_short val);
        Proxy& operator=(u_int val);
        Proxy& operator=(u_long val);
        Proxy& operator=(integer_t val);
        Proxy& operator=(float val);
        Proxy& operator=(number_t val);
        Proxy& operator=(const char *val);
        Proxy& operator=(string& val);
        Proxy& operator=(const CMyPkg& val);
        Proxy& operator=(const CMyPkg* val);
        Proxy& operator=(const Proxy& rhs);
    public:
        // operator[]
        Proxy operator[](const char* key);
        Proxy operator[](int index);
    public:
        operator string() const;
        operator const char*() const;
        operator bool() const;
        operator short() const;
        operator int() const;
        operator long() const;
        operator u_short() const;
        operator u_int() const;
        operator u_long() const;
        operator integer_t() const;
        operator float() const;
        operator number_t() const;
        operator CMyPkg() const;
    public:
        void print(const char* msg = NULL) const;
        int type() const;
        inline bool isNil () const { return type () == MP_T_NIL; }
        inline bool isBoolean () const { return type () == MP_T_BOOLEAN; }
        inline bool isInteger () const { return type () == MP_T_INTEGER; }
        inline bool isNumber () const { return type () == MP_T_NUMBER; }
        inline bool isString () const { return type () == MP_T_STRING; }
        inline bool isCMyPkg () const { return type () == MP_T_OBJECT; }
    };

    class Iterator 
    {
        friend class CMyPkg;
        myiter *iter;
        Iterator& operator=(const Iterator& rhs);
        bool operator==(const Iterator &rhs);
    public:
        explicit Iterator();
        explicit Iterator(const CMyPkg *pkg);
        ~Iterator();
        bool operator!=(const Iterator &rhs);
        const Iterator& operator++();
        const char* key() const;
        Proxy getVal() const;
        const char* getString(size_t &len) const;
    public:
        int type() const;
        inline bool isNil () const { return type () == MP_T_NIL; }
        inline bool isBoolean () const { return type () == MP_T_BOOLEAN; }
        inline bool isInteger () const { return type () == MP_T_INTEGER; }
        inline bool isNumber () const { return type () == MP_T_NUMBER; }
        inline bool isString () const { return type () == MP_T_STRING; }
        inline bool isCMyPkg () const { return type () == MP_T_OBJECT; }
    };

  public:
    CMyPkg();
	CMyPkg(const CMyPkg& rhs);
    CMyPkg& operator=(const CMyPkg& rhs);
	~CMyPkg();

    static CMyPkg ParseJsonFromString(const char *text);
    static CMyPkg ParseJsonFromFile(const char *file);
    void print_json() const;
    void print_json_unformat() const;

	void clear();
    void print(const char* msg = NULL) const;
    bool exist(const char* key);

    /*
     * 序列化
     * @buf 序列化字节缓冲区
     * @return
     * -1   失败
     * >0   字节数
     */
    int pack(CMyBuffer& buf);
    int pack_json(CMyBuffer& buf);
    /*
     * 反序列化
     * @buf 字节缓冲区
     * @len 字节长度
     * @return
     * -1   失败
     * >0   字节数
     */
    int unpack(const CMyBuffer& buf);
    int unpack(const char *buf, size_t len);

    /*
     * CMyPkg迭代器开始
     */
    const Iterator begin() const;
    const Iterator& end() const;

    // add integer
    int addVal(const char *key, char val);
    int addVal(const char *key, bool val);
    int addVal(const char *key, short val);
    int addVal(const char *key, int val);
    int addVal(const char *key, long val);
    int addVal(const char *key, u_short val);
    int addVal(const char *key, u_int val);
    int addVal(const char *key, u_long val);
    int addVal(const char *key, integer_t val);
    int addPointer(const char *key, void *val);
    // add number
    int addVal(const char *key, float val);
    int addVal(const char *key, number_t val);
    // add string
    int addVal(const char *key, const char *val);
    int addVal(const char *key, const char *val, int len);
    int addVal(const char *key, string& val);
    // add CMyPkg
    int addVal(const char *key, const CMyPkg& val);
    int addVal(const char *key, const CMyPkg* val);
    // add Proxy
    int addVal(const char *key, const Proxy& val);

    // append integer
    int appendVal(char val);
    int appendVal(bool val);
    int appendVal(short val);
    int appendVal(int val);
    int appendVal(long val);
    int appendVal(u_short val);
    int appendVal(u_int val);
    int appendVal(u_long val);
    int appendVal(integer_t val);
    // append number
    int appendVal(float val);
    int appendVal(number_t val);
    // append string
    int appendVal(const char *val);
    int appendVal(const char *val, int len);
    int appendVal(string& val);
    // append CMyPkg
    int appendVal(const CMyPkg& val);
    int appendVal(const CMyPkg* val);
    // append Proxy
    int appendVal(const Proxy& val);

    // get 
    Proxy getVal(const char *key) const;
    Proxy getVal(int index) const;
    void* getPointer(const char *key) const;
    const char* getString(const char *key, size_t &len) const;
    int getArraySize() const;

    // operator[]
    Proxy operator[](const char* key);
    Proxy operator[](int index);

    // del field
    int delField(const char *key);

  private:
    CMyPkg(mypkg *pkg);
    int CheckName(const char *name);
    mypkg *pkg;
};

#endif

