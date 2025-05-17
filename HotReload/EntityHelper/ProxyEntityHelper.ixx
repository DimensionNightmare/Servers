module;
export module ProxyEntityHelper;

import ProxyEntity;

export class ProxyEntityHelper : public ProxyEntity
{
private:

	ProxyEntityHelper() = delete;
	~ProxyEntityHelper() = default;

	ProxyEntityHelper(const ProxyEntityHelper&) = delete;
	void operator=(const ProxyEntityHelper&) = delete;

	ProxyEntityHelper(ProxyEntityHelper&&) = delete;
	ProxyEntityHelper& operator=(ProxyEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
};
