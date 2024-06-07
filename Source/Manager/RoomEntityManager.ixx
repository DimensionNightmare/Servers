module;
export module RoomEntityManager;

export import RoomEntity;
import EntityManager;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
public:
	RoomEntityManager() = default;
	~RoomEntityManager() = default;

	virtual bool Init() override;

	virtual void TickMainFrame() override;

public:

};

bool RoomEntityManager::Init()
{
	return EntityManager::Init();
}

void RoomEntityManager::TickMainFrame()
{
}
