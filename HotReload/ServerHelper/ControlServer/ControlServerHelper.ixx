module;
export module ControlServerHelper;

export import DNServer;
import DNServerProxyHelper;
import ServerEntityManagerHelper;

export class ControlServerHelper : public DNServer
{
private:

	ControlServerHelper() = delete;
	~ControlServerHelper() = default;

	ControlServerHelper(const ControlServerHelper&) = delete;
	// void operator=(const ControlServerHelper&) = delete;

	ControlServerHelper(ControlServerHelper&&) = delete;
	ControlServerHelper& operator=(ControlServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ControlServerHelper>;

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	ServerEntityManagerHelper::Ptr GetServerEntityManager() { return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager); }
};
