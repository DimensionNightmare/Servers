module;
export module ServerEntityHelper;

import ServerEntity;

export class ServerEntityHelper : public ServerEntity
{
private:

	ServerEntityHelper() = delete;
	~ServerEntityHelper() = default;

	ServerEntityHelper(const ServerEntityHelper&) = delete;
	void operator=(const ServerEntityHelper&) = delete;

	ServerEntityHelper(ServerEntityHelper&&) = delete;
	ServerEntityHelper& operator=(ServerEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
};
