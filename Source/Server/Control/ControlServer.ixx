module;
#include "hv/hasync.h"
#include "hv/EventLoop.h"

export module ControlServer;

export import DNServer;
import MessagePack;
import ServerEntity;
import EntityManager;

using namespace std;
using namespace hv;

export class ControlServer : public DNServer
{
public:
	ControlServer();

	~ControlServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual EntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;

	EntityManager<ServerEntity>* pEntityMan;
};

module:private;

ControlServer::ControlServer()
{
	emServerType = ServerType::ControlServer;
	pSSock = nullptr;
	pEntityMan = nullptr;
}

ControlServer::~ControlServer()
{
	Stop();
	
	delete pEntityMan;
	pEntityMan = nullptr;

	if(pSSock)
	{
		delete pSSock;
		pSSock = nullptr;
	}
}

bool ControlServer::Init(map<string, string> &param)
{
	if(!param.contains("ip") || !param.contains("port"))
	{
		fprintf(stderr, "%s->ip or port Need! \n", __FUNCTION__);
		return false;
	}

	int port = stoi(param["port"]);
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		fprintf(stderr, "%s->createsocket error \n", __FUNCTION__);
		return false;
	}

    // 输出分配的端口号
	printf("%s->pSSock listen on port %d, listenfd=%d ...\n", __FUNCTION__, pSSock->port, listenfd);

	auto setting = make_shared<unpack_setting_t>();
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting.get());
	pSSock->setThreadNum(4);

	pEntityMan = new EntityManager<ServerEntity>;

	return true;
}

void ControlServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	
}

bool ControlServer::Start()
{
	pSSock->start();
	return true;
}

bool ControlServer::Stop()
{
	pSSock->stop();
	hv::async::cleanup();
	return true;
}

void ControlServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
    while(EventLoopPtr pLoop = pSSock->loop())
	{
        long id = pLoop->tid();
        if(looped.find(id) == looped.end())
        {
            func(pLoop);
            looped[id] = pLoop;
        }
        else
        {
            break;
        }
    };
}