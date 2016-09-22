#pragma once

#include <string>
#include <vector>

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
    Pointer
};

struct Type
{
    std::string name; //Type identifier.
    Primitive primitive; //Primitive type.
    int bitsize; //Size in bits.
    std::string pointto; //Type identifier of *Type
};

struct Member
{
    std::string name; //Member identifier
    std::string type; //Type.name
    int offset; //Member offset in Struct (ignored for unions)
    int arrsize; //Number of elements if Member is an array
};

struct StructUnion
{
    bool isunion = false; //Is this a union?
    int alignment = sizeof(void*); //StructUnion alignment
    std::string name; //StructUnion identifier
    std::vector<Member> members; //StructUnion members
};