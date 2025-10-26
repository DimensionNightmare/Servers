export module ClientEntityHelper;

import ClientEntity;
import ThirdParty.PbGen;
import FuncUtils;

export class ClientEntityHelper : public Helper<ClientEntityHelper, ClientEntity>
{
private:

	ClientEntityHelper() = delete;
	~ClientEntityHelper() = default;

public:

	void SetDbEntity(const std::string& data)
	{
		pDbEntity->ParseFromString(data);
	}

	/// @brief get roomid
	size_t RecordRoomId() { return iRecordRoomId; }
	void SetRecordRoomId(size_t roomId) { iRecordRoomId = roomId; }
};
