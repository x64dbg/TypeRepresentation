#include "Types.h"

int main()
{
    using namespace Types;

    struct ST
    {
        short a;
        short b;
        int y;
    };
    printf("sizeof(ST) = %d\n", sizeof(ST));

    TypeManager t;

    t.AddStruct("ST");
    t.AppendMember("a", "short", 0);
    t.AppendMember("b", "short", 2);
    t.AppendMember("c", "int", 4);
    printf("t.Sizeof(ST) = %d\n", t.Sizeof("ST"));

    t.AddType("DWORD", "unsigned int");
    printf("t.Sizeof(DWORD) = %d\n", t.Sizeof("DWORD"));

    t.AddStruct("_FILETIME", 1);
    t.AppendMember("dwLoDateTime", "DWORD");
    t.AppendMember("dwHighDateTime", "DWORD");
    printf("t.Sizeof(_FILETIME) = %d\n", t.Sizeof("_FILETIME"));

    getchar();
    return 0;
}