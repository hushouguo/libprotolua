/*
 * \file: MessageParser.h
 * \brief: Created by hushouguo at 17:01:33 Aug 09 2017
 */
 
#ifndef __MESSAGE_PARSER_H__
#define __MESSAGE_PARSER_H__

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <exception>
#include <string>
#include <unordered_map>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3003000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3003000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>

#include "lua.hpp"

using namespace google::protobuf;
using namespace google::protobuf::compiler;

class MessageParser
{
	public:
		MessageParser();
		~MessageParser();

	public:
		bool initialize();
		void cleanup();

		bool initialize(const char* filename);/* filename also is directory */

	public:
		bool encodeFromLua(lua_State* L, const char* name, void* buf, size_t& bufsize);
		bool decodeToLua(lua_State* L, const char* name, void* buf, size_t bufsize);

		bool encodeFromLua(lua_State* L, const char* name, std::string& out);
		bool decodeToLua(lua_State* L, const char* name, const std::string& in);
	private:
		DiskSourceTree _tree;
		Importer* _in;
		DynamicMessageFactory _factory;
		std::unordered_map<uint32_t, Message*> _messages;

		class ImporterErrorCollector : public MultiFileErrorCollector 
		{
			public:
				// implements ErrorCollector ---------------------------------------
				void AddError(const std::string& filename, int line, int column,	const std::string& message) override
				{
					fprintf(stderr, "file: %s:%d:%d, error: %s\n", filename.c_str(), line, column, message.c_str());
				}

				void AddWarning(const std::string& filename, int line, int column, const std::string& message) override
				{
					fprintf(stdout, "file: %s:%d:%d, error: %s\n", filename.c_str(), line, column, message.c_str());
				}
		};
		ImporterErrorCollector _errorCollector;

		bool parseProtoFile(const char* filename);
		Message* NewMessage(const char* name);

		bool isDir(const char* file);
		uint32_t hashString(const char* s);
		bool traverseDirectory(const char* folder, const char* filter_suffix, std::function<bool(const char*)>& callback);
	private:
		bool encodeDescriptor(lua_State* L, Message* message, const Descriptor* descriptor, const Reflection* ref);
		bool encodeField(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref);
		bool encodeFieldSimple(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref);
		bool encodeFieldRepeated(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref);

		bool decodeDescriptor(lua_State* L, const Message& message, const Descriptor* descriptor, const Reflection* ref);
		bool decodeField(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref);
		bool decodeFieldSimple(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref);
		bool decodeFieldRepeated(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref, int index);
		void decodeFieldDefaultValue(lua_State* L, const Message& message, const FieldDescriptor* field);
};

#endif