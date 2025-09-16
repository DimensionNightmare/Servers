export module RdbProxyHelper;

import RdbProxy;
import FuncUtils;

export class RdbProxyHelper : public Helper<RdbProxyHelper, RdbProxy>
{
private:

	RdbProxyHelper() = delete;
	~RdbProxyHelper() = default;

public:

	std::shared_ptr<pqxx::connection> GetConnection(uint16_t dbName)
	{
		if (pRdbProxys.contains(dbName))
		{
			return pRdbProxys[dbName];
		}
		return nullptr;
	}

};
