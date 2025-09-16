export module MdbProxyHelper;

import MdbProxy;
import FuncUtils;

export class MdbProxyHelper : public Helper<MdbProxyHelper, MdbProxy>
{
private:

	MdbProxyHelper() = delete;
	~MdbProxyHelper() = default;

public:
	std::shared_ptr<sw::redis::Redis> GetConnection()
	{
		if (pMdbProxys.contains(0))
		{
			return pMdbProxys[0];
		}
		return nullptr;
	}
	
};
