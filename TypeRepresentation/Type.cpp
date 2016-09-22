#include "Types.h"

int main()
{
    using namespace Types;

    struct ST
    {
        char a[3];
        char d;
        int y;
    };
    printf("sizeof(ST) = %d\n", sizeof(ST));

    TypeManager t;

    t.AddStruct("ST");
    t.AppendMember("a", "char", -1, 3);
    t.AppendMember("d", "char");
    t.AppendMember("y", "int");
    printf("t.Sizeof(ST) = %d\n", t.Sizeof("ST"));

    t.AddType("DWORD", "unsigned int");
    printf("t.Sizeof(DWORD) = %d\n", t.Sizeof("DWORD"));

    t.AddStruct("_FILETIME");
    t.AppendMember("dwLoDateTime", "DWORD");
    t.AppendMember("dwHighDateTime", "DWORD");
    printf("t.Sizeof(_FILETIME) = %d\n", t.Sizeof("_FILETIME"));

    getchar();
    return 0;
}