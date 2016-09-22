#include "Type.h"
#include "stringutils.h"
#include <unordered_map>
#include <windows.h>

struct TypeManager
{
    explicit TypeManager()
    {
        setupPrimitives();
    }

    bool AddType(const Type & t)
    {
        if (isDefined(t.name))
            return false;
        auto i = types.insert({ t.name, t });
        return true;
    }

    bool AddType(const std::string & name, Primitive primitive, int bitsize = 0, const std::string & pointto = "")
    {
        if (isDefined(name))
            return false;
        Type t;
        t.name = name;
        t.primitive = primitive;
        auto primsize = primitivesizes[primitive];
        if (bitsize <= 0)
            t.bitsize = primsize;
        else if (bitsize > primsize)
            return false;
        else
            t.bitsize = bitsize;
        t.pointto = pointto;
        return AddType(t);
    }

    bool AddType(const std::string & name, const std::string & primitive)
    {
        auto found = primitives.find(primitive);
        if (found == primitives.end())
            return false;
        return AddType(name, found->second);
    }

    bool AddStruct(const StructUnion & s)
    {
        if (isDefined(s.name))
            return false;
        structs.insert({ s.name, s });
        return true;
    }

    bool AddStruct(const std::string & name, int alignment = 0)
    {
        StructUnion s;
        s.name = name;
        if (alignment > 0)
            s.alignment = alignment;
        return AddStruct(s);
    }

    bool AddUnion(const StructUnion & u)
    {
        if (isDefined(u.name))
            return false;
        structs.insert({ u.name, u });
        return true;
    }

    bool AddUnion(const std::string & name, int alignment = 0)
    {
        StructUnion u;
        u.isunion = true;
        u.name = name;
        if (alignment > 0)
            u.alignment = alignment;
        return AddUnion(u);
    }

    bool AddMember(const std::string & parent, const Member & member)
    {
        auto found = structs.find(parent);
        if (found == structs.end())
            return false;
        found->second.members.push_back(member);
        return true;
    }

    bool AddMember(const std::string & parent, const std::string & name, const std::string & type, int arrsize = 0, int offset = -1)
    {
        auto found = structs.find(parent);
        if (arrsize < 0 || found == structs.end())
            return false;
        const auto & s = found->second;

        for (const auto & member : s.members)
            if (member.name == name)
                return false;

        Member m;
        m.name = name;
        m.arrsize = arrsize;
        m.type = type;
        if (offset < 0)
        {
            m.offset = Sizeof(parent);
            if (s.members.size() && !m.offset)
                return false;
        }
        else
            m.offset = offset;

        return AddMember(parent, m);
    }

#define MAX_DEPTH 100

    int Sizeof(const std::string & type)
    {
        return getSizeof(type, 0);
    }
private:
    std::unordered_map<std::string, Primitive> primitives;
    std::unordered_map<Primitive, int> primitivesizes;
    std::unordered_map<std::string, Type> types;
    std::unordered_map<std::string, StructUnion> structs;

    void setupPrimitives()
    {
        auto p = [this](const std::string & n, Primitive p, int bitsize)
        {
            auto al = StringUtils::Split(n, ',');
            for (const auto & a : al)
                primitives.insert({ a, { p } });
            primitivesizes[p] = bitsize;
        };
        p("int8_t,int8,char,CHAR,INT8,byte,bool,signed char", Int8, 8);
        p("uint8_t,uint8,uchar,unsigned char,UCHAR,UINT8,ubyte,BYTE", Uint8, 8);
        p("int16_t,int16,wchar_t,char16_t,short,SHORT", Int16, 16);
        p("uint16_t,uint16,ushort,USHORT,unsigned short,WORD", Int16, 16);
        p("int32_t,int32,int,long,LONG,INT,INT32", Int32, 32);
        p("uint32_t,uint32,unsigned int,unsigned long,DWORD", Uint32, 32);
        p("int64_t,int64,long long", Int64, 64);
        p("uint64_t,uint64,unsigned long long,QWORD", Uint64, 64);
        p("dsint", Dsint, sizeof(void*) * 8);
        p("duint", Duint, sizeof(void*) * 8);
        p("pointer,PVOID", Pointer, sizeof(void*) * 8);
    }

    template<typename K, typename V>
    static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
    {
        return map.find(k) != map.end();
    }

    bool isDefined(const std::string & id) const
    {
        return mapContains(primitives, id) || mapContains(types, id) || mapContains(structs, id);
    }

    int getSizeof(const std::string & type, int depth)
    {
        if (depth >= MAX_DEPTH)
            return 0;
        auto foundP = primitives.find(type);
        if (foundP != primitives.end())
            return primitivesizes[foundP->second] / 8;
        auto foundT = types.find(type);
        if (foundT != types.end())
        {
            auto bitsize = foundT->second.bitsize;
            auto mod = bitsize % 8;
            if (mod)
                bitsize += 8 - mod;
            return bitsize / 8;
        }
        auto foundS = structs.find(type);
        if (foundS == structs.end() || !foundS->second.members.size())
            return 0;
        const auto & s = foundS->second;
        auto size = 0;
        if (foundS->second.isunion)
        {
            for (const auto & member : s.members)
            {
                auto membersize = getSizeof(member.type, depth + 1) * member.arrsize ? member.arrsize : 1;
                if (!membersize)
                    return 0;
                if (membersize > size)
                    size = membersize;
            }
        }
        else
        {
            const auto & last = s.members[s.members.size() - 1];
            auto lastsize = getSizeof(last.type, depth + 1) * last.arrsize ? last.arrsize : 1;
            if (!lastsize)
                return 0;
            size = last.offset + lastsize;
            auto mod = size % s.alignment;
            if (mod)
                size += s.alignment - mod;
        }
        return size;
    }
};

#ifdef _WIN64
#pragma pack(push, 16)
#else //x86
#pragma pack(push, 8)
#endif //_WIN64

int main()
{
    struct ST
    {
        int x;
        char y;
    };
    printf("sizeof(ST) = %d\n", sizeof(ST));

    TypeManager t;
    t.AddStruct("ST");
    t.AddMember("ST", "x", "int");
    t.AddMember("ST", "y", "char");
    printf("t.Sizeof(ST) = %d\n", t.Sizeof("ST"));

    system("pause");
    return 0;
}