#include "../../../Environment/GameConfig/Gen/Code/schema.pb.h"
#include <fstream>

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
using namespace google::protobuf::io;

#include "google/protobuf/util/json_util.h"
using namespace google::protobuf::util;

int main()
{
	std::ifstream file("D://Project//DimensionNightmare//Environment//GameConfig//Gen//Data//item_weapon.bytes", std::ios::binary);
	if(file.is_open())
	{
		IstreamInputStream input_stream(&file);
		CodedInputStream input(&input_stream);

		GCfg::ItemWeapon tblItem;
		if(tblItem.MergeFromCodedStream(&input))
		{
			auto table = tblItem.mutable_data_list();
			for (auto one : *table)
			{
				std::string str;
				MessageToJsonString(one, &str);
				std::cout <<  str << std::endl << std::endl;
			}
			
		}
		else
		{
			printf("Cant MergeFromCodedStream file~~");
		}

		file.close();
	}
	else
	{
		printf("Cant open file~~");
	}
	return 0;
}