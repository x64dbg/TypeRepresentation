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
        int a = 0xA;
        char b = 0xB;
        struct BLUB
        {
            short c = 0xC;
            int d[2];
        } e;
        int f = 0xF;
    } test;
    test.e.d[0] = 0xD0;
    test.e.d[1] = 0xD1;

    printf("sizeof(TEST) = %d\n", int(sizeof(TEST)));

    t.AddStruct("BLUB", 1);
    t.AppendMember("c", "short");
    t.AppendMember("d", "int", -1, 2);

    t.AddStruct("TEST", 1);
    t.AppendMember("a", "int");
    t.AppendMember("b", "char");
    t.AppendMember("e", "BLUB");
    t.AppendMember("f", "int");
    printf("t.Sizeof(TEST) = %d\n", t.Sizeof("TEST"));

    struct PrintVisitor : TypeManager::Visitor
    {
        explicit PrintVisitor(void* data)
            : mData(data) { }

        bool visitType(const Member & member, const Type & type) override
        {
            unsigned int value = 0;
            if (mData)
                memcpy(&value, (char*)mData + mOffset, type.bitsize / 8);
            indent();
            printf("%s %s = 0x%X;\n", type.name.c_str(), member.name.c_str(), value);
            mOffset += type.bitsize / 8;
            return true;
        }

        bool visitStructUnion(const Member & member, const StructUnion & type) override
        {
            indent();
            printf("%s %s {\n", type.isunion ? "union" : "struct", type.name.c_str());
            mDepth++;
            return true;
        }

        bool visitArray(const Member & member) override
        {
            indent();
            printf("%s[%d] {\n", member.type.c_str(), member.arrsize);
            mDepth++;
            return true;
        }

        bool visitBack(const Member & member) override
        {
            mDepth--;
            indent();
            printf("} %s;\n", member.name.c_str());
            return true;
        }

    private:
        int mDepth = 0;
        int mOffset = 0;
        void* mData = nullptr;

        void indent() const
        {
            printf("%02d: ", mOffset);
            for (auto i = 0; i < mDepth * 2; i++)
                printf(" ");
        }
    } visitor(&test);

    printf("t.Visit(t, TEST) = %s\n", t.Visit("t", "TEST", visitor) ? "true" : "false");

    getchar();
    return 0;
}