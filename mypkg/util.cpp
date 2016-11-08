#include "util.h"

bool IsLetter(char c)
{
    return (c >= 'a' && c <= 'z') 
        || (c >= 'A' && c <= 'Z');
}

bool IsNumber(char c)
{
    return (c >= '0' && c <= '9');
}

bool IsLegal(char c)
{
    return c == '_'
        || c == '@'
        || c == '$'
        ;
}
