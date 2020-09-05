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

static bool 
lua_state_manager_validate(LuaStateManager* lsm)
{
	assert(lsm != nullptr);
	assert(lsm->head != nullptr);

	assert(QueryDepthSList(lsm->head) == lsm->count);

	return true;
}

LuaEngine* 
lua_state_manager_aquire(LuaStateManager* lsm)
{
	assert(lua_state_manager_validate(lsm));

	LuaEngine* lua_engine = nullptr;
	SLIST_ENTRY* list_entry = InterlockedPopEntrySList(lsm->head);

	if (list_entry)
	{
		LuaStateManagerNode* node = (LuaStateManagerNode*)list_entry;
		lua_engine = node->lua_engine;

		InterlockedDecrement(&lsm->count);

		_aligned_free(list_entry);
		list_entry = nullptr;
	}
	else
	{
		lua_engine = lua_engine_create();
	}

	return lua_engine;
}

LuaStateManager* 
lua_state_manager_destroy(LuaStateManager* lsm)
{
	assert(lua_state_manager_validate(lsm));

	SLIST_ENTRY* list_entry = InterlockedPopEntrySList(lsm->head);

	while (list_entry)
	{
		LuaStateManagerNode* node = (LuaStateManagerNode*)list_entry;
		node->lua_engine = lua_engine_destroy(node->lua_engine);

		_aligned_free(list_entry);
		list_entry = InterlockedPopEntrySList(lsm->head);
	}

	_aligned_free(lsm->head);

	free(lsm);
	lsm = nullptr;

	return lsm;
}

LuaStateManager* 
lua_state_manager_create()
{
	LuaStateManager* lsm = (LuaStateManager*)malloc(sizeof LuaStateManager);

	assert(lsm != nullptr);

	if (lsm)
	{
		lsm->count = 0;
		lsm->max_pool_size = LUA_STATE_MAX_POOL_SIZE;

		lsm->head = (PSLIST_HEADER)_aligned_malloc(
			sizeof SLIST_HEADER, 
			MEMORY_ALLOCATION_ALIGNMENT
		);

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

LuaEngine*
lua_state_manager_release(
	LuaStateManager* lsm,
	LuaEngine* lua_engine
)
{
	assert(lua_state_manager_validate(lsm));
	assert(lua_engine != nullptr);

	if (lsm->count >= lsm->max_pool_size)
	{
		lua_engine = lua_engine_destroy(lua_engine);
	}
	else
	{
		LuaStateManagerNode* node = (LuaStateManagerNode*)_aligned_malloc(
			sizeof LuaStateManagerNode, 
			MEMORY_ALLOCATION_ALIGNMENT
		);

		if (node)
		{
			node->lua_engine = lua_engine;

			InterlockedPushEntrySList(lsm->head, &node->item_entry);
			InterlockedIncrement(&lsm->count);
		}
	}

	return nullptr;
}