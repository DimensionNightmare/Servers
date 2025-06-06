module;
export module RdbProxyHelper;

import RdbProxy;

export class RdbProxyHelper : public RdbProxy
{
private:

	RdbProxyHelper() = delete;
	~RdbProxyHelper() = default;

	RdbProxyHelper(const RdbProxyHelper&) = delete;
	void operator=(const RdbProxyHelper&) = delete;

	RdbProxyHelper(RdbProxyHelper&&) = delete;
	RdbProxyHelper& operator=(RdbProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

public:
	using Ptr = std::shared_ptr<RdbProxyHelper>;


	void AddConnection(uint16_t dbName, std::shared_ptr<pqxx::connection>&& connection)
	{
		pMdbProxys.emplace(dbName, std::move(connection));
	}

	std::shared_ptr<pqxx::connection> GetConnection(uint16_t dbName)
	{
		if (pMdbProxys.contains(dbName))
		{
			return pMdbProxys[dbName];
		}
		return nullptr;
	}
	
};
