module;

export module GlobalMessage;

export import GlobalControl;

export import GlobalServer;

static GlobalServer* PGlobalServer = nullptr;
export GlobalServer* GetGlobalServer(GlobalServer* server = nullptr)
{
	if(server)
	{
		PGlobalServer = server;
	}

	return PGlobalServer;
}