module;
#include "hv/HttpServer.h"

export module DNWebProxy;

export class DNWebProxy : public hv::HttpServer
{
public:
	DNWebProxy(){};
	~DNWebProxy(){};

public: // dll override
};

module:private;