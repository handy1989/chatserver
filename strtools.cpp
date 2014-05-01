#include "strtools.h"

char *r_strip(char* &str)
{
    if (NULL == str)
    {
        return NULL;
    }
    char *p = str + strlen(str) - 1;
    char filter_chs[] = " \t\n";
    while (p >= str && strchr(filter_chs, *p))
    {
        *p-- = 0;
    }
    return str;
}

char *l_strip(char* &str)
{
    if (NULL == str)
    {
        return NULL;
    }
    char *p = str;
    char filter_chs[] = " \t\n";
    while (*p && strchr(filter_chs, *p))
    {
        *p++ = 0;
    }
    str = p;
    return str;
}

char *strip(char* &str)
{
    r_strip(str);
    l_strip(str);
    return str;
}
