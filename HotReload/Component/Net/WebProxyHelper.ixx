export module WebProxyHelper;

import WebProxy;

export class WebProxyHelper : public WebProxy
{
	
private:

	WebProxyHelper() = delete;
	~WebProxyHelper() = default;

	WebProxyHelper(const WebProxyHelper&) = delete;
	void operator=(const WebProxyHelper&) = delete;

	WebProxyHelper(WebProxyHelper&&) = delete;
	WebProxyHelper& operator=(WebProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<WebProxyHelper>;
	using CVPtr = const Ptr&;
	
};
