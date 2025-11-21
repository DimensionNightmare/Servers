
module;
#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/json/json.h"

#include "l10n/l10n.pb.h"
export module ThirdParty.Protobuf;

using namespace google::protobuf;

export
{

	using ::Map;
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
	using ::EL10nCode_Name;
}

// common 

export namespace l10n
{
	using l10n::l10nCode;
	using l10n::l10nCodes;
}

export namespace Proto
{

	auto FindMessageTypeByName(auto name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }

	auto GetPrototype(auto descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }

}
