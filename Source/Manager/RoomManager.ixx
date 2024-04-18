module;
export module RoomManager;

export import RoomEntity;
import EntityManager;

export template<class TEntity = RoomEntity>
class RoomManager : public EntityManager<RoomEntity> 
{
public:
	RoomManager(){}
	~RoomManager(){}

	virtual bool Init() override;

public:

};

template <class TEntity>
bool RoomManager<TEntity>::Init()
{
	return EntityManager<TEntity>::Init();
}
