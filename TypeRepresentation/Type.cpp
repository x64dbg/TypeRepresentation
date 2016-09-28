#include "Types.h"

using namespace Types;

struct PrintVisitor : TypeManager::Visitor
{
    explicit PrintVisitor(void* data = nullptr)
        : mData(data) { }

    bool visitType(const Member & member, const Type & type) override
    {
        unsigned long long value = 0;
        if (mData)
            memcpy(&value, (char*)mData + mOffset, type.bitsize / 8);
        else
            value = 0xCC;
        indent();
        if (parent().type == Parent::Array)
            printf("%s %s[%d] = 0x%llX;\n", type.name.c_str(), member.name.c_str(), parent().index++, value);
        else
            printf("%s %s = 0x%llX;\n", type.name.c_str(), member.name.c_str(), value);
        if (parent().type != Parent::Union)
            mOffset += type.bitsize / 8;
        return true;
    }

    bool visitStructUnion(const Member & member, const StructUnion & type) override
    {
        indent();
        printf("%s %s {\n", type.isunion ? "union" : "struct", type.name.c_str());
        mParentData.push_back(Parent(type.isunion ? Parent::Union : Parent::Struct));
        return true;
    }

    bool visitArray(const Member & member) override
    {
        indent();
        printf("%s[%d] {\n", member.type.c_str(), member.arrsize);
        mParentData.push_back(Parent(Parent::Array));
        return true;
    }

    bool visitBack(const Member & member) override
    {
        mParentData.pop_back();
        indent();
        printf("} %s;\n", member.name.c_str());
        return true;
    }

private:
    struct Parent
    {
        enum Type
        {
            Struct,
            Union,
            Array
        };
        
        Type type;
        int index;

        explicit Parent(Type type)
            : type(type), index(0) { }
    };

    Parent & parent()
    {
        return mParentData[mParentData.size() - 1];
    }

    std::vector<Parent> mParentData;

    int mOffset = 0;
    void* mData = nullptr;

    void indent() const
    {
        printf("%02d: ", mOffset);
        for (auto i = 0; i < int(mParentData.size()) * 2; i++)
            printf(" ");
    }
};

int main()
{
    TypeManager t;
    PrintVisitor visitor;

    struct ST
    {
        char a[3];
        int y;
    };
    printf("sizeof(ST) = %d\n", int(sizeof(ST)));

    t.AddStruct("ST");
    t.AppendMember("a", "char", 3);
    t.AppendMember("y", "int", 0, 4);
    printf("t.Sizeof(ST) = %d\n", t.Sizeof("ST"));

    printf("t.Visit(t, ST) = %d\n", t.Visit("t", "ST", visitor = PrintVisitor()));

    puts("- - - -");

    t.AddType("DWORD", "unsigned int");
    printf("t.Sizeof(DWORD) = %d\n", t.Sizeof("DWORD"));

    t.AddStruct("_FILETIME");
    t.AppendMember("dwLoDateTime", "DWORD");
    t.AppendMember("dwHighDateTime", "DWORD");
    printf("t.Sizeof(_FILETIME) = %d\n", t.Sizeof("_FILETIME"));

    puts("- - - -");

#pragma pack(1)
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

    printf("t.Visit(t, UT) = %d\n", t.Visit("t", "UT", visitor = PrintVisitor()));

    puts("- - - -");

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

    t.AddStruct("BLUB");
    t.AppendMember("c", "short");
    t.AppendMember("d", "int", 2);

    t.AddStruct("TEST");
    t.AppendMember("a", "int");
    t.AppendMember("b", "char");
    t.AppendMember("e", "BLUB");
    t.AppendMember("f", "int");
    printf("t.Sizeof(TEST) = %d\n", t.Sizeof("TEST"));

    printf("t.Visit(t, TEST) = %d\n", t.Visit("t", "TEST", visitor = PrintVisitor(&test)));

    getchar();
    return 0;
}