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
	using CVPtr = const Ptr&;

	std::shared_ptr<pqxx::connection> GetConnection(uint16_t dbName)
	{
		if (pRdbProxys.contains(dbName))
		{
			return pRdbProxys[dbName];
		}
		return nullptr;
	}

};
