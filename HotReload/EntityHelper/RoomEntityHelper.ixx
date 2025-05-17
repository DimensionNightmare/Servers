module;
export module RoomEntityHelper;

import RoomEntity;

export class RoomEntityHelper : public RoomEntity
{
private:

	RoomEntityHelper() = delete;
	~RoomEntityHelper() = default;

	RoomEntityHelper(const RoomEntityHelper&) = delete;
	void operator=(const RoomEntityHelper&) = delete;

	RoomEntityHelper(RoomEntityHelper&&) = delete;
	RoomEntityHelper& operator=(RoomEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
};
