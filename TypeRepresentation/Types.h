#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Types
{
    enum Primitive
    {
        Int8,
        Uint8,
        Int16,
        Uint16,
        Int32,
        Uint32,
        Int64,
        Uint64,
        Dsint,
        Duint,
        Pointer,
        Float,
        Double
    };

    struct Type
    {
        std::string name; //Type identifier.
        Primitive primitive; //Primitive type.
        int bitsize = 0; //Size in bits.
        std::string pointto; //Type identifier of *Type
    };

    struct Member
    {
        std::string name; //Member identifier
        std::string type; //Type.name
        int arrsize = 0; //Number of elements if Member is an array
    };

    struct StructUnion
    {
        bool isunion = false; //Is this a union?
        int size = 0;
        std::string name; //StructUnion identifier
        std::vector<Member> members; //StructUnion members
    };

    struct TypeManager
    {
        explicit TypeManager()
        {
            setupPrimitives();
        }

        bool AddType(const std::string & name, const std::string & type)
        {
            auto found = types.find(type);
            if (found == types.end())
                return false;
            return AddType(name, found->second.primitive);
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
            return addType(t);
        }

        bool AddStruct(const std::string & name)
        {
            StructUnion s;
            s.name = name;
            return addStructUnion(s);
        }

        bool AddUnion(const std::string & name)
        {
            StructUnion u;
            u.isunion = true;
            u.name = name;
            return addStructUnion(u);
        }

        bool AppendMember(const std::string & name, const std::string & type, int arrsize = 0, int offset = -1)
        {
            return AddMember(laststruct, name, type, arrsize, offset);
        }

        bool AddMember(const std::string & parent, const std::string & name, const std::string & type, int arrsize = 0, int offset = -1)
        {
            auto found = structs.find(parent);
            if (arrsize < 0 || found == structs.end() || !isDefined(type))
                return false;
            auto & s = found->second;

            for (const auto & member : s.members)
                if (member.name == name)
                    return false;

            auto typeSize = Sizeof(type);
            if (arrsize)
                typeSize *= arrsize;

            Member m;
            m.name = name;
            m.arrsize = arrsize;
            m.type = type;

            if (offset >= 0) //user-defined offset
            {
                if (offset < s.size)
                    return false;
                if (offset > s.size)
                {
                    Member pad;
                    pad.type = "char";
                    pad.arrsize = offset - s.size;
                    char padname[32] = "";
                    sprintf_s(padname, "padding%d", pad.arrsize);
                    pad.name = padname;
                    s.members.push_back(m);
                    s.size += pad.arrsize;
                }
            }

            s.members.push_back(m);

            if (s.isunion)
            {
                if (typeSize > s.size)
                    s.size = typeSize;
            }
            else
            {
                s.size += typeSize;
            }
            return true;
        }

        int Sizeof(const std::string & type)
        {
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
            if (foundS == structs.end())
                return 0;
            return foundS->second.size;
        }

        struct Visitor
        {
            virtual ~Visitor() { }
            virtual bool visitType(const Member & member, const Type & type) = 0;
            virtual bool visitStructUnion(const Member & member, const StructUnion & type) = 0;
            virtual bool visitArray(const Member & member) = 0;
            virtual bool visitBack(const Member & member) = 0;
        };

        bool Visit(const std::string & name, const std::string & type, Visitor & visitor)
        {
            Member m;
            m.name = name;
            m.type = type;
            return visitMember(m, visitor);
        }

    private:
        std::unordered_map<Primitive, int> primitivesizes;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, StructUnion> structs;
        std::string laststruct;

        void setupPrimitives()
        {
            auto p = [this](const std::string & n, Primitive p, int bitsize)
            {
                std::string a;
                for (auto ch : n)
                {
                    if (ch == ',')
                    {
                        if (a.length())
                        {
                            Type t;
                            t.bitsize = bitsize;
                            t.name = a;
                            t.primitive = p;
                            types.insert({ a, t });
                        }
                        a.clear();
                    }
                    else
                        a.push_back(ch);
                }
                if (a.length())
                {
                    Type t;
                    t.bitsize = bitsize;
                    t.name = a;
                    t.primitive = p;
                    types.insert({ a, t });
                }
                primitivesizes[p] = bitsize;
            };
            p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char) * 8);
            p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char) * 8);
            p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short) * 8);
            p("uint16_t,uint16,ushort,unsigned short", Int16, sizeof(unsigned short) * 8);
            p("int32_t,int32,int,long", Int32, sizeof(int) * 8);
            p("uint32_t,uint32,unsigned int,unsigned long", Uint32, sizeof(unsigned int) * 8);
            p("int64_t,int64,long long", Int64, sizeof(long long) * 8);
            p("uint64_t,uint64,unsigned long long", Uint64, sizeof(unsigned long long) * 8);
            p("dsint", Dsint, sizeof(void*) * 8);
            p("duint,size_t", Duint, sizeof(void*) * 8);
            p("ptr,void*", Pointer, sizeof(void*) * 8);
            p("float", Float, sizeof(float) * 8);
            p("double", Double, sizeof(double) * 8);
        }

        template<typename K, typename V>
        static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
        {
            return map.find(k) != map.end();
        }

        bool isDefined(const std::string & id) const
        {
            return mapContains(types, id) || mapContains(structs, id);
        }

        bool addStructUnion(const StructUnion & s)
        {
            laststruct = s.name;
            if (isDefined(s.name))
                return false;
            structs.insert({ s.name, s });
            return true;
        }

        bool addType(const Type & t)
        {
            if (isDefined(t.name))
                return false;
            types.insert({ t.name, t });
            return true;
        }

        bool visitMember(const Member & root, Visitor & visitor)
        {
            auto foundT = types.find(root.type);
            if (foundT != types.end())
                return visitor.visitType(root, foundT->second);
            auto foundS = structs.find(root.type);
            if (foundS != structs.end())
            {
                const auto & s = foundS->second;
                if (!visitor.visitStructUnion(root, s))
                    return false;
                for (const auto & child : s.members)
                {
                    if (child.arrsize)
                    {
                        if (!visitor.visitArray(child))
                            return false;
                        for (auto i = 0; i < child.arrsize; i++)
                            if (!visitMember(child, visitor))
                                return false;
                        if (!visitor.visitBack(child))
                            return false;
                    }
                    else if (!visitMember(child, visitor))
                        return false;
                }
                return visitor.visitBack(root);
            }
            return true;
        }
    };
};