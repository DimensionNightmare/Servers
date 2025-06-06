module;

#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/extension_set.h"

#include "l10n/l10n.pb.h"

export module ThirdParty.Protobuf;

export
{
	using namespace google::protobuf;

	using ::Message;
	using ::Descriptor;
	using ::FieldDescriptor;
	using ::Reflection;
	using ::FieldOptions;

	using ::json::MessageToJsonString;
	using ::ShutdownProtobufLibrary;

	using ::EL10nCode;
	using ::ELogLevel;
	using ::EL10nType;
	using ::EL10nCode_IsValid;
	using ::ELogLevel_Parse;
	using ::EL10nType_Parse;
	

	enum CustomFieldOptions
	{
		e_primary_key = 1,
		e_len_limit = 2,
		e_unique = 3,
		e_default = 4,
		e_datetime = 5,
		e_autogen = 6,
	};

}

// common 

export namespace l10n
{

	using l10n::l10nCode;
	using l10n::l10nCodes;
}

export namespace PbGen
{

	auto EL10nCode_Name_(EL10nCode param){return EL10nCode_Name(param);}

	auto FindMessageTypeByName(auto name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }

	auto GetPrototype(auto descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }

}
