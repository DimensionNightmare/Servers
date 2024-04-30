module;
#include "hv/HttpServer.h"
export module DNWebProxy;

using namespace hv;

export class DNWebProxy : public HttpServer
{
public:
	DNWebProxy(){};
	~DNWebProxy(){};

public: // dll override
};

