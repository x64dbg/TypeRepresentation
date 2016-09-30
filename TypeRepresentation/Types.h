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
        std::string owner; //Type owner
        std::string name; //Type identifier.
        std::string pointto; //Type identifier of *Type
        Primitive primitive; //Primitive type.
        int size = 0; //Size in bytes.
    };

    struct Member
    {
        std::string name; //Member identifier
        std::string type; //Type.name
        int arrsize = 0; //Number of elements if Member is an array
    };

    struct StructUnion
    {
        std::string owner; //StructUnion owner
        std::string name; //StructUnion identifier
        std::vector<Member> members; //StructUnion members
        bool isunion = false; //Is this a union?
        int size = 0;
    };

    enum CallingConvention
    {
        Cdecl,
        Stdcall,
        Thiscall,
        Delphi
    };

    struct Function
    {
        std::string owner; //Function owner
        std::string name; //Function identifier
        std::string rettype; //Function return type
        CallingConvention callconv; //Function calling convention
        bool noreturn; //Function does not return (ExitProcess, _exit)
        std::vector<Member> args; //Function arguments
    };

    struct TypeManager
    {
        explicit TypeManager()
        {
            setupPrimitives();
        }

        bool AddType(const std::string & owner, const std::string & name, const std::string & type)
        {
            auto found = types.find(type);
            if (found == types.end())
                return false;
            return AddType(owner, name, found->second.primitive);
        }

        bool AddType(const std::string & owner, const std::string & name, Primitive primitive, const std::string & pointto = "")
        {
            if (owner.empty() || name.empty() || isDefined(name))
                return false;
            Type t;
            t.owner = owner;
            t.name = name;
            t.primitive = primitive;
            t.size = primitivesizes[primitive];
            t.pointto = pointto;
            return addType(t);
        }

        bool AddStruct(const std::string & owner, const std::string & name)
        {
            StructUnion s;
            s.name = name;
            s.owner = owner;
            return addStructUnion(s);
        }

        bool AddUnion(const std::string & owner, const std::string & name)
        {
            StructUnion u;
            u.owner = owner;
            u.name = name;
            u.isunion = true;
            return addStructUnion(u);
        }

        bool AppendMember(const std::string & name, const std::string & type, int arrsize = 0, int offset = -1)
        {
            return AddMember(laststruct, name, type, arrsize, offset);
        }

        bool AddMember(const std::string & parent, const std::string & name, const std::string & type, int arrsize = 0, int offset = -1)
        {
            auto found = structs.find(parent);
            if (!isDefined(type) && !validPtr(type))
                return false;
            if (arrsize < 0 || found == structs.end() || !isDefined(type) || name.empty() || type.empty() || type == parent)
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

        bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, CallingConvention callconv = Cdecl, bool noreturn = false)
        {
            auto found = functions.find(name);
            if (found != functions.end() || name.empty() || owner.empty())
                return false;
            lastfunction = name;
            Function f;
            f.owner = owner;
            f.name = name;
            f.rettype = rettype;
            f.callconv = callconv;
            f.noreturn = noreturn;
            functions.insert({ f.name, f });
            return true;
        }

        bool AddArg(const std::string & function, const std::string & name, const std::string & type)
        {
            auto found = functions.find(function);
            if (!isDefined(type) && !validPtr(type))
                return false;
            if (found == functions.end() || function.empty() || name.empty() || !isDefined(type))
                return false;
            lastfunction = function;
            Member arg;
            arg.name = name;
            arg.type = type;
            found->second.args.push_back(arg);
            return true;
        }

        bool AppendArg(const std::string & name, const std::string & type)
        {
            return AddArg(lastfunction, name, type);
        }

        int Sizeof(const std::string & type)
        {
            auto foundT = types.find(type);
            if (foundT != types.end())
                return foundT->second.size;
            auto foundS = structs.find(type);
            if (foundS != structs.end())
                return foundS->second.size;
            return 0;
        }

        struct Visitor
        {
            virtual ~Visitor() { }
            virtual bool visitType(const Member & member, const Type & type) = 0;
            virtual bool visitStructUnion(const Member & member, const StructUnion & type) = 0;
            virtual bool visitArray(const Member & member) = 0;
            virtual bool visitPtr(const Member & member, const Type & type) = 0;
            virtual bool visitBack(const Member & member) = 0;
        };

        bool Visit(const std::string & name, const std::string & type, Visitor & visitor)
        {
            Member m;
            m.name = name;
            m.type = type;
            return visitMember(m, visitor);
        }

        void Clear(const std::string & owner = "")
        {
            laststruct.clear();
            lastfunction.clear();
            filterOwnerMap(types, owner);
            filterOwnerMap(structs, owner);
            filterOwnerMap(functions, owner);
        }

    private:
        std::unordered_map<Primitive, int> primitivesizes;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, StructUnion> structs;
        std::unordered_map<std::string, Function> functions;
        std::string laststruct;
        std::string lastfunction;

        template<typename K, typename V>
        void filterOwnerMap(std::unordered_map<K ,V> & map, const std::string & owner)
        {
            for (auto i = map.begin(); i != map.end();)
            {
                auto j = i++;
                if (j->second.owner.empty())
                    continue;
                if (owner.empty() || j->second.owner == owner)
                    map.erase(j);
            }
        }

        void setupPrimitives()
        {
            auto p = [this](const std::string & n, Primitive p, int size)
            {
                std::string a;
                for (auto ch : n)
                {
                    if (ch == ',')
                    {
                        if (!a.empty())
                        {
                            Type t;
                            t.size = size;
                            t.name = a;
                            t.primitive = p;
                            types.insert({ a, t });
                        }
                        a.clear();
                    }
                    else
                        a.push_back(ch);
                }
                if (!a.empty())
                {
                    Type t;
                    t.size = size;
                    t.name = a;
                    t.primitive = p;
                    types.insert({ a, t });
                }
                primitivesizes[p] = size;
            };
            p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char));
            p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char));
            p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short));
            p("uint16_t,uint16,ushort,unsigned short", Int16, sizeof(unsigned short));
            p("int32_t,int32,int,long", Int32, sizeof(int));
            p("uint32_t,uint32,unsigned int,unsigned long", Uint32, sizeof(unsigned int));
            p("int64_t,int64,long long", Int64, sizeof(long long));
            p("uint64_t,uint64,unsigned long long", Uint64, sizeof(unsigned long long));
            p("dsint", Dsint, sizeof(void*));
            p("duint,size_t", Duint, sizeof(void*));
            p("ptr,void*", Pointer, sizeof(void*));
            p("float", Float, sizeof(float));
            p("double", Double, sizeof(double));
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

        bool validPtr(const std::string & id)
        {
            if (id[id.length() - 1] == '*')
            {
                auto type = id.substr(0, id.length() - 1);
                if (!isDefined(type))
                    return false;
                return AddType("ptr", id, Pointer, type);
            }
            return false;
        }

        bool addStructUnion(const StructUnion & s)
        {
            laststruct = s.name;
            if (s.owner.empty() || s.name.empty() || isDefined(s.name))
                return false;
            structs.insert({ s.name, s });
            return true;
        }

        bool addType(const Type & t)
        {
            if (t.owner.empty() || t.name.empty() || isDefined(t.name))
                return false;
            types.insert({ t.name, t });
            return true;
        }

        bool visitMember(const Member & root, Visitor & visitor)
        {
            auto foundT = types.find(root.type);
            if (foundT != types.end())
            {
                const auto & t = foundT->second;
                if (!t.pointto.empty())
                {
                    if (!isDefined(t.pointto))
                        return false;
                    if (visitor.visitPtr(root, t)) //allow the visitor to bail out
                    {
                        if (!Visit("*" + root.name, t.pointto, visitor))
                            return false;
                        return visitor.visitBack(root);
                    }
                    return true;
                }
                return visitor.visitType(root, t);
            }
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
            return false;
        }
    };
};