module;
export module MdbProxyHelper;

import MdbProxy;

export class MdbProxyHelper : public MdbProxy
{
private:

	MdbProxyHelper() = delete;
	~MdbProxyHelper() = default;

	MdbProxyHelper(const MdbProxyHelper&) = delete;
	void operator=(const MdbProxyHelper&) = delete;

	MdbProxyHelper(MdbProxyHelper&&) = delete;
	MdbProxyHelper& operator=(MdbProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

public:
	using Ptr = std::shared_ptr<MdbProxyHelper>;
	using CVPtr = const Ptr&;

	std::shared_ptr<sw::redis::Redis> GetConnection()
	{
		if (pMdbProxys.contains(0))
		{
			return pMdbProxys[0];
		}
		return nullptr;
	}
	
};
