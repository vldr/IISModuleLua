#include "shared.h"

struct _LuaStateManager
{
	unsigned int count;
	unsigned int max_pool_size;

	SLIST_HEADER* head;
};

typedef struct _LuaStateManagerNode 
{
	SLIST_ENTRY item_entry;
	LuaEngine* lua_engine;
} LuaStateManagerNode;

static bool lua_state_manager_validate(LuaStateManager* lsm)
{
	assert(lsm != nullptr);
	assert(lsm->head != nullptr);

	return true;
}

LuaStateManager* lua_state_manager_start()
{
	LuaStateManager* lsm = (LuaStateManager*)malloc(sizeof LuaStateManager);

	assert(lsm != nullptr);

	if (lsm)
	{
		lsm->count = 0;
		lsm->max_pool_size = LUA_STATE_MAX_POOL_SIZE;

		lsm->head = (PSLIST_HEADER)_aligned_malloc(sizeof SLIST_HEADER, MEMORY_ALLOCATION_ALIGNMENT);

		assert(lsm->head != nullptr);

		if (!lsm->head)
		{
			free(lsm);
			lsm = nullptr;
		}
		else
		{
			InitializeSListHead(lsm->head);

			assert(lua_state_manager_validate(lsm));
		}
	}

	return lsm;
}


