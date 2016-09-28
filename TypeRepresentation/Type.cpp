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
    printf("sizeof(ST) = %d\n", int(sizeof(ST)));

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

    union UT
    {
        char a;
        short b;
        int c;
        long long d;
    };
    printf("sizeof(UT) = %d\n", int(sizeof(UT)));

    t.AddUnion("UT");
    t.AppendMember("a", "char");
    t.AppendMember("b", "short");
    t.AppendMember("c", "int");
    t.AppendMember("d", "long long");
    printf("t.Sizeof(UT) = %d\n", t.Sizeof("UT"));

#pragma pack(1)
    struct TEST
    {
        int a;
        char b;
        struct BLUB
        {
            short c;
            int d[2];
        } x;
        int y;
    };
    printf("sizeof(TEST) = %d\n", int(sizeof(TEST)));

    t.AddStruct("BLUB", 1);
    t.AppendMember("c", "short");
    t.AppendMember("d", "int", -1, 2);

    t.AddStruct("TEST", 1);
    t.AppendMember("a", "int");
    t.AppendMember("b", "char");
    t.AppendMember("x", "BLUB");
    t.AppendMember("y", "int");
    printf("t.Sizeof(TEST) = %d\n", t.Sizeof("TEST"));

    struct PrintVisitor : TypeManager::Visitor
    {
        bool visitType(const Member & member, const Type & type) override
        {
            indent();
            printf("%s %s;\n", type.name.c_str(), member.name.c_str());
            return true;
        }

        bool visitStructUnion(const Member & member, const StructUnion & type) override
        {
            indent();
            printf("%s %s {\n", type.isunion ? "union" : "struct", type.name.c_str());
            depth++;
            return true;
        }

        bool visitArray(const Member & member) override
        {
            indent();
            printf("%s[%d] {\n", member.type.c_str(), member.arrsize);
            depth++;
            return true;
        }

        bool visitBack(const Member & member) override
        {
            depth--;
            indent();
            printf("} %s;\n", member.name.c_str());
            return true;
        }

    private:
        int depth = 0;

        void indent() const
        {
            for (auto i = 0; i < depth * 4; i++)
                printf(" ");
        }
    } visitor;

    printf("t.Visit(t, TEST) = %s\n", t.Visit("t", "TEST", visitor) ? "true" : "false");

    getchar();
    return 0;
}