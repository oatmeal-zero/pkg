#include <assert.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include "mypkg.h"

using namespace std;
typedef unsigned long long myclock_t;

inline myclock_t nowclock()
{
    struct timeval tv; 
    gettimeofday(&tv, NULL);
    return (myclock_t)tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

CMyPkg test_json(const char *file)
{
    CMyPkg pkg = CMyPkg::ParseJsonFromFile(file);
    pkg.print("++json2pkg++");
    return pkg;
}

CMyPkg test_array()
{
    CMyPkg Students;
    Students["Ids"]  = CMyPkg();
    Students["Names"] = CMyPkg();
    for (int idx=0; idx<10; idx++)
    {
        char name[128]{0};
        sprintf(name, "name%d", idx);
        Students["Ids"][idx] = idx+10;
        Students["Names"][idx] = name;
    }

    Students["Ids"].print("test_print_array");

    Students.print("++test_array++");

    return Students;
}

void test_print_array(CMyPkg pkg)
{
    printf("++test_print_array++\n");
    CMyPkg Ids = pkg.getVal("Ids");
    int size = Ids.getArraySize();
    for (int idx = 0; idx < size; idx++)
    {
        int id = Ids.getVal(idx);
        printf("%d ", id);
    }
    printf("\n");
}

void test_operator()
{
    CMyPkg pkg;
    pkg["int"] = 123;
    pkg["string"] = "Terminates the last protected function called and returns message as the error object. Function error never returns.";
    pkg[0] = 1234567;
    pkg[4] = "平常我们用的基本上都是基本类型元素作为";
    pkg[3] = 3.145;

    pkg.print("++test_operator++");
}

void test_base(CMyPkg& pkg)
{
    pkg.addVal("boolean", true);
    pkg.addVal("boolean2", false);
    pkg.addVal("int", 123);
    pkg.addVal("string", "hello, world!");
    pkg.addVal("iiiii", -1124123123);
    pkg.addVal("int1", 123);
    pkg.addVal("int2", 123456);
    pkg.addVal("int3", 123456789);
    pkg.addVal("int4", 123456789123456789);
    pkg.addVal("int5", -123);
    pkg.addVal("int6", -123456);
    pkg.addVal("int7", -123456789);
    pkg.addVal("int8", -123456789123456789);
    pkg.addVal("int9", 'Z');

    // 测试字符串数组
    pkg.appendVal("hello");
    pkg.appendVal("world");
    pkg.appendVal("!");

    pkg.appendVal(1111);
    pkg.appendVal(222);
    pkg.appendVal(51123);
    pkg.print("==================");

    bool b = pkg["boolean"];
    bool b2 = pkg["boolean2"];
    printf("boolean:%d %d\n", b, b2);
}

void test_mypkg()
{
    CMyPkg pkg;
    int ret;
    const char *val;
    ret = pkg.addVal("string", "at most five decimal digits");
    val = pkg.getVal("string");
    printf("ret=%d val=%s\n", ret, val);

    ret = pkg.addVal("string", "Non-conforming compilers should use a value with");
    val = pkg.getVal("string");
    printf("ret=%d val=%s\n", ret, val);

    ret = pkg.addVal("string2", "测试字符串数组");
    val = pkg.getVal("string2");
    printf("ret=%d val=%s\n", ret, val);

    ret = pkg.addVal("string2", "__cplusplus宏在C++标准中的描述如下");
    val = pkg.getVal("string2");
    printf("ret=%d val=%s\n", ret, val);

    val = pkg.getVal("string2");
    printf("ret=%d ++val=%s\n", ret, val);

    pkg.addVal("integer", -1121231231);
    int valint = pkg.getVal("integer");
    unsigned int valuint = pkg.getVal("integer");
    printf("++val=%d %u\n", valint, valuint);

    CMyPkg pkg2(pkg);

    printf("exist:%d\n", pkg.exist("string"));

    pkg.delField("string2");

    printf("exist:%d\n", pkg.exist("string"));
    pkg["selfpkg"] = pkg;

    pkg.print();

    pkg.clear();
    pkg.print();

    pkg2.print("=====pkg2======");
}

void test_base_function()
{
    CMyPkg pkg;
    // integer
    pkg.addVal("key1", 'Z');
    short sval = -123;
    pkg.addVal("key2", sval);
    int ival = -1234567;
    pkg.addVal("key3", ival);
    long lval = -456789098765432231;
    pkg.addVal("key4", lval);
    u_short usval = 123;
    pkg.addVal("key5", usval);
    u_int uival = 1234567;
    pkg.addVal("key6", uival);
    u_long ulval = 456789098765432231;
    pkg.addVal("key7", ulval);
    // 支持的最大整数
    pkg.addVal("longint", 9223372036854775807);
    // number
    float fval = 3.1415926;
    pkg.addVal("key8", fval);
    double dval = 3.1415926535897932384626433832795;
    pkg.addVal("key9", dval);
    // string
    pkg.addVal("key10", "The reference manual is the official definition of the Lua language. ");
    // pkg
    CMyPkg pkg2;
    pkg2.addVal("key1", "由站长工具推出的在线JSON编辑器是一个基于网络的工具来解析,压缩,格式化,查看,编辑等智能化编辑器");
    pkg.addVal("key11", pkg2);
    pkg2.print("++++++++pkg2++++++++");
    pkg.print("++++++++pkg1++++++++");
}

void test_iterator(CMyPkg& pkg)
{
    printf("test iterator begin.\n");
    for (auto iter = pkg.begin(); iter != pkg.end(); ++iter)
    {
        switch (iter.type()) {
            case MP_T_INTEGER:
                {
                    integer_t val = iter.getVal();
                    printf("key:%s ival:%lld\n",
                            iter.key(), val);
                    break;
                }
            case MP_T_NUMBER:
                {
                    number_t val = iter.getVal();
                    printf("key:%s nval:%.8lf\n",
                            iter.key(), val);
                    break;
                }
            case MP_T_STRING:
                {
                    size_t len;
                    const char* val = iter.getString(len);
                    printf("key:%s sval:%s %lu\n",
                            iter.key(), val, len);
                    break;
                }
            case MP_T_OBJECT:
                {
                    CMyPkg tmp = iter.getVal();
                    tmp.print("test_iterator's pkg");
                    break;
                }
            default:
                printf("undefine type:%d\n", iter.type());
        }
    }
    printf("test iterator end.\n");
}

void test_cjson()
{
    char text1[]="{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}";
    char text2[]="[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	char text3[]="[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	char text4[]="{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}";
	char text5[]="[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]";
    CMyPkg pkg1 = CMyPkg::ParseJsonFromString(text1);
    pkg1.print("test_cjson");
    pkg1.print_json();

    CMyPkg pkg2 = CMyPkg::ParseJsonFromString(text2);
    pkg2.print("test_cjson");
    pkg2.print_json();

    CMyPkg pkg3 = CMyPkg::ParseJsonFromString(text3);
    pkg3.print("test_cjson");
    pkg3.print_json();

    CMyPkg pkg4 = CMyPkg::ParseJsonFromString(text4);
    pkg4.print("test_cjson");
    pkg4.print_json();

    CMyPkg pkg5 = CMyPkg::ParseJsonFromString(text5);
    pkg5.print("test_cjson");
    pkg5.print_json();
}

int main(int argc, char** argv)
{
    myclock_t begin = nowclock();
    printf("sizeof pkg:%lu\n", sizeof(CMyPkg));
    CMyPkg pkg;
    pkg.addVal("key", "hello, world!");
    pkg.addVal("number", 3.1415926);
    pkg.addVal("copykey", pkg.getVal("key"));

    const char *notexist = pkg.getVal("copykey");
    printf("notexist:%s\n", notexist);

    CMyPkg pkg2;
    pkg2 = pkg;
    pkg2.print();

    CMyPkg students = test_array();
    test_print_array(students);
    test_base(pkg);

    pkg.addVal("pkg", pkg2);
    pkg["pkgcir"] = pkg;
    pkg.print();

    CMyPkg tmp = pkg.getVal("pkg");
    tmp.addVal("newkey", 1234567);
    tmp.print("tmp pkg print");
    pkg.print("*****************");

    test_iterator(pkg);
    pkg.print();

    test_mypkg();
    test_base_function();

    test_operator();
    test_json("test.json");

    test_cjson();

    myclock_t end = nowclock();
    printf("test end. total cost millisecond(s):%llu\n", (end - begin));
    return 0;
}
