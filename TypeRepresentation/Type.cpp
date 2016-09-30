#include "Types.h"

using namespace Types;

struct PrintVisitor : TypeManager::Visitor
{
    explicit PrintVisitor(void* data = nullptr, int maxPtrDepth = 0)
        : mData(data), mMaxPtrDepth(maxPtrDepth) { }

    bool visitType(const Member & member, const Type & type) override
    {
        unsigned long long value = 0;
        if (mData)
            memcpy(&value, (char*)mData + mOffset, size_t(type.size));
        else
            value = 0xCC;
        indent();
        if (parent().type == Parent::Array)
            printf("%s %s[%d] = 0x%llX;", type.name.c_str(), member.name.c_str(), parent().index++, value);
        else
            printf("%s %s = 0x%llX;", type.name.c_str(), member.name.c_str(), value);
        puts(type.pointto.empty() || mPtrDepth >= mMaxPtrDepth ? "" : " {");
        if (parent().type != Parent::Union)
            mOffset += type.size;
        return true;
    }

    bool visitStructUnion(const Member & member, const StructUnion & type) override
    {
        indent();
        printf("%s %s {\n", type.isunion ? "union" : "struct", type.name.c_str());
        mParents.push_back(Parent(type.isunion ? Parent::Union : Parent::Struct));
        return true;
    }

    bool visitArray(const Member & member) override
    {
        indent();
        printf("%s[%d] {\n", member.type.c_str(), member.arrsize);
        mParents.push_back(Parent(Parent::Array));
        return true;
    }

    bool visitPtr(const Member & member, const Type & type) override
    {
        auto offset = mOffset;
        auto res = visitType(member, type); //print the pointer value
        if (mPtrDepth >= mMaxPtrDepth)
            return false;
        void* value = nullptr;
        if (mData)
            memcpy(&value, (char*)mData + offset, size_t(type.size));
        else
            return false;
        mParents.push_back(Parent(Parent::Pointer));
        parent().offset = mOffset;
        parent().data = mData;
        mOffset = 0;
        mData = value;
        mPtrDepth++;
        return res;
    }

    bool visitBack(const Member & member) override
    {
        if (parent().type == Parent::Pointer)
        {
            mOffset = parent().offset;
            mData = parent().data;
            mPtrDepth--;
        }
        mParents.pop_back();
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
            Array,
            Pointer
        };
        
        Type type;
        int index = 0;
        void* data = nullptr;
        int offset = 0;

        explicit Parent(Type type)
            : type(type) { }
    };

    Parent & parent()
    {
        return mParents[mParents.size() - 1];
    }

    void indent() const
    {
        printf("%p:%02d: ", mData, mOffset);
        for (auto i = 0; i < int(mParents.size()) * 2; i++)
            printf(" ");
    }

    std::vector<Parent> mParents;
    int mOffset = 0;
    void* mData = nullptr;
    int mPtrDepth = 0;
    int mMaxPtrDepth = 0;
};

#pragma pack(push, 1)
int main()
{
    TypeManager t;
    PrintVisitor visitor;
    std::string owner = "me";

    struct ST
    {
        char a[3];
        int y;
    };
    printf("sizeof(ST) = %d\n", int(sizeof(ST)));

    t.AddStruct(owner, "ST");
    t.AppendMember("a", "char", 3);
    t.AppendMember("y", "int", 0, 4);
    printf("t.Sizeof(ST) = %d\n", t.Sizeof("ST"));

    printf("t.Visit(t, ST) = %d\n", t.Visit("t", "ST", visitor = PrintVisitor()));

    puts("- - - -");

    t.AddType(owner, "DWORD", "unsigned int");
    printf("t.Sizeof(DWORD) = %d\n", t.Sizeof("DWORD"));

    t.AddStruct(owner, "_FILETIME");
    t.AppendMember("dwLoDateTime", "DWORD");
    t.AppendMember("dwHighDateTime", "DWORD");
    printf("t.Sizeof(_FILETIME) = %d\n", t.Sizeof("_FILETIME"));

    puts("- - - -");

    union UT
    {
        char a;
        short b;
        int c;
        long long d;
    };
    printf("sizeof(UT) = %d\n", int(sizeof(UT)));

    t.AddUnion(owner, "UT");
    t.AppendMember("a", "char");
    t.AppendMember("b", "short");
    t.AppendMember("c", "int");
    t.AppendMember("d", "long long");
    printf("t.Sizeof(UT) = %d\n", t.Sizeof("UT"));

    printf("t.Visit(t, UT) = %d\n", t.Visit("t", "UT", visitor = PrintVisitor()));

    puts("- - - -");

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

    t.AddStruct(owner, "BLUB");
    t.AppendMember("c", "short");
    t.AppendMember("d", "int", 2);

    t.AddStruct(owner, "TEST");
    t.AppendMember("a", "int");
    t.AppendMember("b", "char");
    t.AppendMember("e", "BLUB");
    t.AppendMember("f", "int");
    printf("t.Sizeof(TEST) = %d\n", t.Sizeof("TEST"));

    printf("t.Visit(t, TEST) = %d\n", t.Visit("t", "TEST", visitor = PrintVisitor(&test)));

    puts("- - - -");

    struct POINTEE
    {
        int n = 0x1337;
        TEST t;
    } ptee;
    ptee.t = test;

    struct POINTER
    {
        int x = 0x30;
        POINTEE* p = nullptr;
        int y = 0x70;
    } ptr;
    ptr.p = &ptee;

    t.AddStruct(owner, "POINTEE");
    t.AppendMember("n", "int");
    t.AppendMember("t", "TEST");

    t.AddStruct(owner, "POINTER");
    t.AppendMember("x", "int");
    t.AppendMember("p", "POINTEE*");
    t.AppendMember("y", "int");

    printf("t.Visit(ptr, POINTER) = %d\n", t.Visit("ptr", "POINTER", visitor = PrintVisitor(&ptr, 1)));

    puts("- - - -");

    struct LIST_ENTRY
    {
        int x = 0x123;
        LIST_ENTRY* next;
        int y = 0x312;
    } le;
    le.next = &le;

    t.AddStruct(owner, "LIST_ENTRY");
    t.AppendMember("x", "int");
    t.AppendMember("next", "LIST_ENTRY*");
    t.AppendMember("y", "int");

    printf("t.Visit(le, LIST_ENTRY) = %d\n", t.Visit("le", "LIST_ENTRY", visitor = PrintVisitor(&le, 6)));

    t.AddType(owner, "const char", "char");

    t.AddFunction(owner, "strcasecmp", "int", Cdecl);
    t.AppendArg("s1", "const char*");
    t.AppendArg("s2", "const char*");

    t.Clear();

    getchar();
    return 0;
}