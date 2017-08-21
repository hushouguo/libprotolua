/*
 * \file: MessageParser.cpp
 * \brief: Created by hushouguo at 17:06:00 Aug 09 2017
 */

#include "MessageParser.h"

MessageParser::MessageParser()
{
	this->initialize();
}

MessageParser::~MessageParser()
{
	cleanup();
}

bool MessageParser::initialize()
{
	this->_tree.MapPath("", "./");
	this->_in = new Importer(&this->_tree, &this->_errorCollector);
	return true;
}

void MessageParser::cleanup()
{
	for (auto& i : this->_messages)
	{
		delete i.second;
	}
	this->_messages.clear();

	delete this->_in;
}

bool MessageParser::parseProtoFile(const char* filename)
{
	const FileDescriptor* fileDescriptor = this->_in->Import(filename);
	if (!fileDescriptor)
	{
		fprintf(stderr, "import file: %s failure\n", filename);
		return false;
	}
	return true;
}

bool MessageParser::initialize(const char* filename)
{
	std::function<bool(const char*)> func = [this](const char* fullname)->bool
	{
		return this->parseProtoFile(fullname);
	};

	return traverseDirectory(filename, "*.proto", std::ref(func));
}


Message* MessageParser::NewMessage(const char* name)
{
	uint32_t id = hashString(name);
	auto i = this->_messages.find(id);
	if (i != this->_messages.end())
	{
		i->second->Clear();
		return i->second;
	}

	const Descriptor* descriptor = this->_in->pool()->FindMessageTypeByName(name);
	if (!descriptor)
	{
		fprintf(stderr, "not found descriptor for message: %s\n", name);
		return nullptr;
	}
	
	const Message* prototype = this->_factory.GetPrototype(descriptor);
	if (!prototype)
	{
		fprintf(stderr, "not found prototype for message\n");
		return nullptr;
	}

	Message* message = prototype->New();
	this->_messages.insert(std::make_pair(id, message));

	fprintf(stdout, "message:%s\n", message->DebugString().c_str());

	return message;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

bool MessageParser::encodeFieldRepeated(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref)
{
	assert(field->is_repeated());

	bool rc = true;

	if (!lua_isnoneornil(L, -1))
	{
		switch (field->cpp_type())
		{
#define CASE_FIELD_TYPE(CPPTYPE, METHOD_TYPE, VALUE_TYPE, VALUE)	\
			case google::protobuf::FieldDescriptor::CPPTYPE_##CPPTYPE: {\
				VALUE_TYPE value = VALUE;\
				ref->Add##METHOD_TYPE(message, field, value);\
			}break;
			CASE_FIELD_TYPE(INT32, Int32, int32_t, lua_tointeger(L, -1));// TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
			CASE_FIELD_TYPE(INT64, Int64, int64_t, lua_tointeger(L, -1));// TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
			CASE_FIELD_TYPE(UINT32, UInt32, uint32_t, lua_tointeger(L, -1));// TYPE_UINT32, TYPE_FIXED32
			CASE_FIELD_TYPE(UINT64, UInt64, uint64_t, lua_tointeger(L, -1));// TYPE_UINT64, TYPE_FIXED64
			CASE_FIELD_TYPE(DOUBLE, Double, double, lua_tonumber(L, -1));// TYPE_DOUBLE
			CASE_FIELD_TYPE(FLOAT, Float, float, lua_tonumber(L, -1));// TYPE_FLOAT
			CASE_FIELD_TYPE(BOOL, Bool, bool, lua_toboolean(L, -1));// TYPE_BOOL
			CASE_FIELD_TYPE(ENUM, EnumValue, int, lua_tointeger(L, -1));// TYPE_ENUM
			CASE_FIELD_TYPE(STRING, String, std::string, lua_tostring(L, -1));// TYPE_STRING, TYPE_BYTES
#undef CASE_FIELD_TYPE

			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: // TYPE_MESSAGE, TYPE_GROUP
				{
					Message* submessage = ref->AddMessage(message, field, &this->_factory);
					rc = this->encodeDescriptor(L, submessage, field->message_type(), submessage->GetReflection());
				}
				break;
		}
	}

	return rc;
}


bool MessageParser::encodeFieldSimple(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref)
{
	assert(!field->is_repeated());

	bool rc = true;

	if (!lua_isnoneornil(L, -1))
	{
		switch (field->cpp_type())
		{
#define CASE_FIELD_TYPE(CPPTYPE, METHOD_TYPE, VALUE_TYPE, VALUE)	\
			case google::protobuf::FieldDescriptor::CPPTYPE_##CPPTYPE: {\
				VALUE_TYPE value = VALUE;\
				ref->Set##METHOD_TYPE(message, field, value);\
			}break;
			CASE_FIELD_TYPE(INT32, Int32, int32_t, lua_tointeger(L, -1));// TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
			CASE_FIELD_TYPE(INT64, Int64, int64_t, lua_tointeger(L, -1));// TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
			CASE_FIELD_TYPE(UINT32, UInt32, uint32_t, lua_tointeger(L, -1));// TYPE_UINT32, TYPE_FIXED32
			CASE_FIELD_TYPE(UINT64, UInt64, uint64_t, lua_tointeger(L, -1));// TYPE_UINT64, TYPE_FIXED64
			CASE_FIELD_TYPE(DOUBLE, Double, double, lua_tonumber(L, -1));// TYPE_DOUBLE
			CASE_FIELD_TYPE(FLOAT, Float, float, lua_tonumber(L, -1));// TYPE_FLOAT
			CASE_FIELD_TYPE(BOOL, Bool, bool, lua_toboolean(L, -1));// TYPE_BOOL
			CASE_FIELD_TYPE(ENUM, EnumValue, int, lua_tointeger(L, -1));// TYPE_ENUM
			CASE_FIELD_TYPE(STRING, String, std::string, lua_tostring(L, -1));// TYPE_STRING, TYPE_BYTES
#undef CASE_FIELD_TYPE

			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: // TYPE_MESSAGE, TYPE_GROUP
				{
					Message* submessage = ref->MutableMessage(message, field, &this->_factory);
					rc = this->encodeDescriptor(L, submessage, field->message_type(), submessage->GetReflection());
				}
				break;
		}
	}
	
	return rc;
}

bool MessageParser::encodeField(lua_State* L, Message* message, const FieldDescriptor* field, const Reflection* ref)
{
	bool rc = true;
	if (field->is_repeated())
	{
		lua_pushstring(L, field->name().c_str());/* push key */
		lua_gettable(L, -2);

		if (lua_istable(L, -1))
		{
			int table_index = lua_gettop(L);

			lua_pushnil(L);
			while (lua_next(L, table_index) != 0 && rc)
			{
				/* 'key' is at index -2 and 'value' at index -1, here, `value` is a table */
				//if (!lua_isnumber(L, -2))/* Integer key */
				//{
				//	alarm_log("ignore not-integer key for field:%s\n", field->name().c_str());
				//}
				//ignore `key` type

				rc = this->encodeFieldRepeated(L, message, field, ref);

				lua_pop(L, 1);/* removes 'value'; keeps 'key' for next iteration */
			}
		}

		lua_pop(L, 1);/* remove `table` or nil */
	}
	else
	{
		lua_pushstring(L, field->name().c_str());/* key */
		lua_gettable(L, -2);

		rc = this->encodeFieldSimple(L, message, field, ref);

		lua_pop(L, 1);/* remove `value` or nil */
	}

	return rc;
}

bool MessageParser::encodeDescriptor(lua_State* L, Message* message, const Descriptor* descriptor, const Reflection* ref)
{
	if (!lua_istable(L, -1))
	{
		fprintf(stderr, "stack top not table for message: %s\n", message->GetTypeName().c_str());
		return false;
	}

	int field_count = descriptor->field_count();
	for (int i = 0; i < field_count; ++i)
	{
		const FieldDescriptor* field = descriptor->field(i);
		if (!this->encodeField(L, message, field, ref))
		{
			fprintf(stderr, "encodeField: %s for message:%s failure\n", field->name().c_str(), message->GetTypeName().c_str());
			return false;
		}
	}

	return true;
}

bool MessageParser::encodeFromLua(lua_State* L, const char* name, void* buf, size_t& bufsize)
{
	const Descriptor* descriptor = this->_in->pool()->FindMessageTypeByName(name);
	if (!descriptor)
	{
		fprintf(stderr, "not found descriptor for message: %s\n", name);
		return false;
	}

	Message* message = this->NewMessage(name);
	if (!message)
	{
		return false;
	}

	assert(message->ByteSize() == 0);

	try
	{
		if (!this->encodeDescriptor(L, message, descriptor, message->GetReflection()))
		{
			fprintf(stderr, "encodeDescriptor failure for message: %s\n", name);
			return false;
		}
	}
	catch(std::exception& e)
	{
		fprintf(stderr, "encodeDescriptor exception:%s\n", e.what());
		return false;
	}

	size_t byteSize = message->ByteSize();
	if (byteSize > bufsize)
	{
		fprintf(stderr, "bufsize: %ld(need: %ld) overflow for message: %s\n", bufsize, byteSize, name);
		return false;
	}

	if (!message->SerializeToArray(buf, byteSize))
	{
		fprintf(stderr, "Serialize message:%s failure, byteSize:%ld\n", name, byteSize);
		return false;
	}

	bufsize = byteSize;
	return true;
}

bool MessageParser::encodeFromLua(lua_State* L, const char* name, std::string& out)
{
	const Descriptor* descriptor = this->_in->pool()->FindMessageTypeByName(name);
	if (!descriptor)
	{
		fprintf(stderr, "not found descriptor for message: %s\n", name);
		return false;
	}

	Message* message = this->NewMessage(name);
	if (!message)
	{
		return false;
	}

	assert(message->ByteSize() == 0);

	try
	{
		if (!this->encodeDescriptor(L, message, descriptor, message->GetReflection()))
		{
			fprintf(stderr, "encodeDescriptor failure for message: %s\n", name);
			return false;
		}
	}
	catch(std::exception& e)
	{
		fprintf(stderr, "encodeDescriptor exception:%s\n", e.what());
		return false;
	}

	size_t byteSize = message->ByteSize();

	if (!message->SerializeToString(&out))
	{
		fprintf(stderr, "Serialize message:%s failure, byteSize:%ld\n", name, byteSize);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------------

void MessageParser::decodeFieldDefaultValue(lua_State* L, const Message& message, const FieldDescriptor* field)
{
	if (field->is_repeated())
	{
		lua_pushstring(L, field->name().c_str());
		lua_newtable(L);
		lua_settable(L, -3);/* push foo={} */
	}
	else
	{
		switch (field->cpp_type())
		{
#define CASE_FIELD_TYPE(CPPTYPE, METHOD)	\
			case google::protobuf::FieldDescriptor::CPPTYPE_##CPPTYPE: {\
				lua_pushstring(L, field->name().c_str());\
				METHOD;\
				lua_settable(L, -3);\
			}break;

			CASE_FIELD_TYPE(INT32, lua_pushinteger(L, 0));// TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
			CASE_FIELD_TYPE(INT64, lua_pushinteger(L, 0));// TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
			CASE_FIELD_TYPE(UINT32, lua_pushinteger(L, 0));// TYPE_UINT32, TYPE_FIXED32
			CASE_FIELD_TYPE(UINT64, lua_pushinteger(L, 0));// TYPE_UINT64, TYPE_FIXED64
			CASE_FIELD_TYPE(DOUBLE, lua_pushnumber(L, 0.0f));// TYPE_DOUBLE
			CASE_FIELD_TYPE(FLOAT, lua_pushnumber(L, 0.0f));// TYPE_FLOAT
			CASE_FIELD_TYPE(BOOL, lua_pushboolean(L, false));// TYPE_BOOL
			CASE_FIELD_TYPE(ENUM, lua_pushinteger(L, 0));// TYPE_ENUM
			CASE_FIELD_TYPE(STRING, lua_pushstring(L, ""));// TYPE_STRING, TYPE_BYTES
#undef CASE_FIELD_TYPE

			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: // TYPE_MESSAGE, TYPE_GROUP
				{
					lua_pushstring(L, field->name().c_str()); /* key */
					lua_newtable(L);				
					lua_settable(L, -3);/* push foo={} */
				}
				break;
		}
	}
}

bool MessageParser::decodeFieldRepeated(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref, int index)
{
	assert(field->is_repeated());

	bool rc = true;
	switch (field->cpp_type())
	{
#define CASE_FIELD_TYPE(CPPTYPE, METHOD_TYPE, VALUE_TYPE, METHOD)	\
			case google::protobuf::FieldDescriptor::CPPTYPE_##CPPTYPE: {\
				lua_pushinteger(L, index);\
				VALUE_TYPE value = ref->GetRepeated##METHOD_TYPE(message, field, index);\
				METHOD(L, value);\
				lua_settable(L, -3);\
			}break;

		CASE_FIELD_TYPE(INT32, Int32, int32_t, lua_pushinteger);// TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
		CASE_FIELD_TYPE(INT64, Int64, int64_t, lua_pushinteger);// TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
		CASE_FIELD_TYPE(UINT32, UInt32, uint32_t, lua_pushinteger);// TYPE_UINT32, TYPE_FIXED32
		CASE_FIELD_TYPE(UINT64, UInt64, uint64_t, lua_pushinteger);// TYPE_UINT64, TYPE_FIXED64
		CASE_FIELD_TYPE(DOUBLE, Double, double, lua_pushnumber);// TYPE_DOUBLE
		CASE_FIELD_TYPE(FLOAT, Float, float, lua_pushnumber);// TYPE_FLOAT
		CASE_FIELD_TYPE(BOOL, Bool, bool, lua_pushboolean);// TYPE_BOOL
		CASE_FIELD_TYPE(ENUM, EnumValue, int, lua_pushinteger);// TYPE_ENUM
		//CASE_FIELD_TYPE(STRING, String, std::string, lua_pushstring);// TYPE_STRING, TYPE_BYTES
#undef CASE_FIELD_TYPE

		case google::protobuf::FieldDescriptor::CPPTYPE_STRING: // TYPE_STRING, TYPE_BYTES
			{
				std::string value = ref->GetRepeatedString(message, field, index);
				lua_pushinteger(L, index);
				lua_pushstring(L, value.c_str());
				lua_settable(L, -3);
			}
			break;

		case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: // TYPE_MESSAGE, TYPE_GROUP
			{
				lua_pushinteger(L, index);
				lua_newtable(L);

				const Message& submessage = ref->GetRepeatedMessage(message, field, index);
				rc = this->decodeDescriptor(L, submessage, field->message_type(), submessage.GetReflection());

				lua_settable(L, -3);
			}
			break;
	}

	return rc;
}

bool MessageParser::decodeFieldSimple(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref)
{
	assert(!field->is_repeated());

	bool rc = true;
	switch (field->cpp_type())
	{
#define CASE_FIELD_TYPE(CPPTYPE, METHOD_TYPE, VALUE_TYPE, METHOD)	\
			case google::protobuf::FieldDescriptor::CPPTYPE_##CPPTYPE: {\
				lua_pushstring(L, field->name().c_str());\
				if (ref->HasField(message, field)) {\
					VALUE_TYPE value = ref->Get##METHOD_TYPE(message, field);\
					METHOD(L, value);\
				}else{\
					this->decodeFieldDefaultValue(L, message, field);\
				}\
				lua_settable(L, -3);\
			}break;

		CASE_FIELD_TYPE(INT32, Int32, int32_t, lua_pushinteger);// TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
		CASE_FIELD_TYPE(INT64, Int64, int64_t, lua_pushinteger);// TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
		CASE_FIELD_TYPE(UINT32, UInt32, uint32_t, lua_pushinteger);// TYPE_UINT32, TYPE_FIXED32
		CASE_FIELD_TYPE(UINT64, UInt64, uint64_t, lua_pushinteger);// TYPE_UINT64, TYPE_FIXED64
		CASE_FIELD_TYPE(DOUBLE, Double, double, lua_pushnumber);// TYPE_DOUBLE
		CASE_FIELD_TYPE(FLOAT, Float, float, lua_pushnumber);// TYPE_FLOAT
		CASE_FIELD_TYPE(BOOL, Bool, bool, lua_pushboolean);// TYPE_BOOL
		CASE_FIELD_TYPE(ENUM, EnumValue, int, lua_pushinteger);// TYPE_ENUM
		//CASE_FIELD_TYPE(STRING, String, std::string, lua_pushstring);// TYPE_STRING, TYPE_BYTES
#undef CASE_FIELD_TYPE

		case google::protobuf::FieldDescriptor::CPPTYPE_STRING: // TYPE_STRING, TYPE_BYTES
			{
				lua_pushstring(L, field->name().c_str());
				std::string value = ref->GetString(message, field);
				lua_pushstring(L, value.c_str());
				lua_settable(L, -3);
			}
			break;

		case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: // TYPE_MESSAGE, TYPE_GROUP
			{
				lua_pushstring(L, field->name().c_str()); /* key */
				lua_newtable(L);

				const Message& submessage = ref->GetMessage(message, field, &this->_factory);
				rc = this->decodeDescriptor(L, submessage, field->message_type(), submessage.GetReflection());

				lua_settable(L, -3);
			}
			break;
	}

	return rc;
}

bool MessageParser::decodeField(lua_State* L, const Message& message, const FieldDescriptor* field, const Reflection* ref)
{
	bool rc = true;
	if (field->is_repeated())
	{
		lua_pushstring(L, field->name().c_str());
		lua_newtable(L);

		for (int i = 0; rc && i < ref->FieldSize(message, field); ++i)
		{
			rc = this->decodeFieldRepeated(L, message, field, ref, i);
		}

		lua_settable(L, -3);
	}
	else
	{
		rc = this->decodeFieldSimple(L, message, field, ref);
	}

	return rc;
}

bool MessageParser::decodeDescriptor(lua_State* L, const Message& message, const Descriptor* descriptor, const Reflection* ref)
{
	int field_count = descriptor->field_count();
	for (int i = 0; i < field_count; ++i)
	{
		const FieldDescriptor* field = descriptor->field(i);

		if (!field->is_repeated() && !ref->HasField(message, field))
		{
			this->decodeFieldDefaultValue(L, message, field);
			continue;
		}/* fill default value to lua when a non-repeated field not set, for message field */

		if (!this->decodeField(L, message, field, ref))
		{
			fprintf(stderr, "decodeField: %s for message:%s failure\n", field->name().c_str(), message.GetTypeName().c_str());
			return false;
		}
	}
	return true;
}

bool MessageParser::decodeToLua(lua_State* L, const char* name, void* buf, size_t bufsize)
{
	const Descriptor* descriptor = this->_in->pool()->FindMessageTypeByName(name);
	if (!descriptor)
	{
		fprintf(stderr, "not found descriptor for message: %s\n", name);
		return false;
	}

	Message* message = this->NewMessage(name);
	if (!message)
	{
		return false;
	}

	assert(message->ByteSize() == 0);

	if (!message->ParseFromArray(buf, bufsize))
	{
		fprintf(stderr, "Unserialize message:%s failure, byteSize:%ld\n", name, bufsize);
		return false;
	}

	lua_newtable(L);
	try
	{
		if (!this->decodeDescriptor(L, *message, descriptor, message->GetReflection()))
		{
			fprintf(stderr, "decodeDescriptor failure for message: %s\n", name);
			return false;
		}
	}
	catch(std::exception& e)
	{
		fprintf(stderr, "decodeDescriptor exception:%s\n", e.what());
		return false;
	}

	return true;
}

bool MessageParser::decodeToLua(lua_State* L, const char* name, const std::string& in)
{
	const Descriptor* descriptor = this->_in->pool()->FindMessageTypeByName(name);
	if (!descriptor)
	{
		fprintf(stderr, "not found descriptor for message: %s\n", name);
		return false;
	}

	Message* message = this->NewMessage(name);
	if (!message)
	{
		return false;
	}

	assert(message->ByteSize() == 0);

	if (!message->ParseFromString(in))
	{
		fprintf(stderr, "Unserialize message:%s failure, byteSize:%ld\n", name, in.length());
		return false;
	}

	lua_newtable(L);
	try
	{
		if (!this->decodeDescriptor(L, *message, descriptor, message->GetReflection()))
		{
			fprintf(stderr, "decodeDescriptor failure for message: %s\n", name);
			return false;
		}
	}
	catch(std::exception& e)
	{
		fprintf(stderr, "decodeDescriptor exception:%s\n", e.what());
		return false;
	}

	return true;
}

bool MessageParser::traverseDirectory(const char* folder, const char* filter_suffix, std::function<bool(const char*)>& callback)
{
	if (!isDir(folder))
	{ 
		return callback(folder);
	}

	DIR* dir = opendir(folder);

	struct dirent* ent;
	while ((ent = readdir(dir)) != nullptr)
	{
		if (ent->d_name[0] == '.') { continue; } //filter hide file

		if (filter_suffix != nullptr)
		{
			char* suffix = strrchr(ent->d_name, '.');//filter not .proto suffix file 
			if (!suffix || strcasecmp(suffix, filter_suffix) != 0) 
			{ 
				continue; 
			}
		}

		char fullname[PATH_MAX];
		snprintf(fullname, sizeof(fullname), "%s/%s", folder, ent->d_name);
		if (ent->d_type & DT_DIR)
		{
			return traverseDirectory(fullname, filter_suffix, callback);
		}
		else
		{
			if (!callback(fullname)) { return false; }
		}
	}

	return true;
}

uint32_t MessageParser::hashString(const char* s)
{
	uint32_t h = 0, g;
	const char* end = s + strlen(s);
	while (s < end)
	{
		h = (h << 4) + *s++;
		if ((g = (h & 0xF0000000)))
		{
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return h;
}

bool MessageParser::isDir(const char* file)
{
	struct stat buf;	
	if (stat(file, &buf) != 0) { return false; }	
	return S_ISDIR(buf.st_mode);	
}

