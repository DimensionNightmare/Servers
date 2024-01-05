module;
#include "hv/TcpServer.h"

export module DNServerProxy;

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy(){};
	~DNServerProxy(){};

public: // dll override
};

module:private;