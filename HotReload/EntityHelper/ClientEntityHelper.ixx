module;
export module ClientEntityHelper;

import ClientEntity;

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
	

	void SetDbEntity(const std::string& data)
	{
		pDbEntity = std::make_shared<GDb::Player>();
		pDbEntity->ParseFromString(data);
	}
};
