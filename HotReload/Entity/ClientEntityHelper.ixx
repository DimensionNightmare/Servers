module;
export module ClientEntityHelper;

import ClientEntity;
import ThirdParty.PbGen;

export class ClientEntityHelper : public ClientEntity
{
private:

	ClientEntityHelper() = delete;
	~ClientEntityHelper() = default;

	ClientEntityHelper(const ClientEntityHelper&) = delete;
	void operator=(const ClientEntityHelper&) = delete;

	ClientEntityHelper(ClientEntityHelper&&) = delete;
	ClientEntityHelper& operator=(ClientEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

public:
	using Ptr = std::shared_ptr<ClientEntityHelper>;
	using CVPtr = const Ptr&;
	

	void SetDbEntity(const std::string& data)
	{
		pDbEntity->ParseFromString(data);
	}

	/// @brief get roomid
	uint64_t RecordRoomId() { return iRecordRoomId; }
	void SetRecordRoomId(uint64_t roomId) { iRecordRoomId = roomId; }
};
