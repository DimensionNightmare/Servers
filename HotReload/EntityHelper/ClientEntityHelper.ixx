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
};
