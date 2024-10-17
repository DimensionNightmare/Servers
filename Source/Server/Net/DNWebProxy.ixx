module;
export module DNWebProxy;

import ThirdParty.Libhv;

export class DNWebProxy : public HttpServer
{

public:

	DNWebProxy() = default;

	~DNWebProxy() = default;

	int Start()
	{
		return start();
	}

	void End()
	{
		stop();
	}
};
