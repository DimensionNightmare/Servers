module;
export module DNWebProxyHelper;

import DNWebProxy;

export class DNWebProxyHelper : public DNWebProxy
{
	
private:

	DNWebProxyHelper() = delete;
	~DNWebProxyHelper() = default;

	DNWebProxyHelper(const DNWebProxyHelper&) = delete;
	void operator=(const DNWebProxyHelper&) = delete;

	DNWebProxyHelper(DNWebProxyHelper&&) = delete;
	DNWebProxyHelper& operator=(DNWebProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<DNWebProxyHelper>;
	
};
