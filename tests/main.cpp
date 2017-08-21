/*
 * \file: main.cpp
 * \brief: Created by hushouguo at 16:54:01 Aug 17 2017
 */

#include "MessageParser.h"

MessageParser parser;

static int protobuf_encode(lua_State *L) 
{
	int args = lua_gettop(L);
	if (args < 2)
	{
		fprintf(stderr, "`protobuf_encode` need 2 params");
		return 0;
	}

	const char* name = lua_tostring(L, -args);

	std::string out;
	if (!parser.encodeFromLua(L, name, out))
	{
		fprintf(stderr, "encodeFromLua failure");
		return 0;
	}

	lua_pushstring(L, out.c_str());
	return 1;
}

static int protobuf_decode(lua_State *L) 
{
	int args = lua_gettop(L);
	if (args < 2)
	{
		fprintf(stderr, "`protobuf_decode` need 2 params");
		return 0;
	}

	const char* name = lua_tostring(L, -args);
	std::string in = lua_tostring(L, -args+1);

	if (!parser.decodeToLua(L, name, in))
	{
		fprintf(stderr, "decodeToLua failure");
		return 0;
	}

	return 1;
}

int main()
{
	parser.initialize("addressbook.proto");
	lua_State* L = luaL_newstate();	
	luaL_openlibs(L);
	lua_register(L, "protobuf_encode", protobuf_encode);
	lua_register(L, "protobuf_decode", protobuf_decode);
	luaL_dofile(L, "./test.lua");
	lua_close(L);
	return 0;
}

