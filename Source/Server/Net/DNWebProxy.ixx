module;
export module DNWebProxy;

import ThirdParty.Libhv;

export class DNWebProxy : public HttpServer
{
public:
	DNWebProxy() = default;
	~DNWebProxy() = default;

	int Start();

	void End();
public: // dll override
};

int DNWebProxy::Start()
{
	return start();
}

void DNWebProxy::End()
{
	stop();
}
