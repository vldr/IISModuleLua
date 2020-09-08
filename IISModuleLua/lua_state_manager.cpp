#include "shared.h"

typedef struct _LuaStateManager
{
	IHttpServer* http_server;
	SLIST_HEADER* head;
} LuaStateManager;

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
	assert(lsm->http_server != nullptr);

	return true;
}

LuaEngine* 
lua_state_manager_aquire(LuaStateManager* lsm)
{
	assert(lua_state_manager_validate(lsm));

	LuaEngine* lua_engine = nullptr;

	if (lsm && lsm->http_server)
	{
		SLIST_ENTRY* list_entry = InterlockedPopEntrySList(lsm->head);

		if (list_entry)
		{
			LuaStateManagerNode* node = (LuaStateManagerNode*)list_entry;
			lua_engine = node->lua_engine;

			assert(lua_engine_get_list_entry(lua_engine) == list_entry);
			assert(lua_engine == node->lua_engine);
		}
		else
		{
			lua_engine = lua_engine_create(lsm->http_server->GetAppPoolName());
		}
	}

	return lua_engine;
}

LuaStateManager* 
lua_state_manager_destroy(LuaStateManager* lsm)
{
	assert(lua_state_manager_validate(lsm));

	if (lsm && lsm->head)
	{
		SLIST_ENTRY* list_entry = InterlockedPopEntrySList(lsm->head);

		while (list_entry)
		{
			LuaStateManagerNode* node = (LuaStateManagerNode*)list_entry;
			node->lua_engine = lua_engine_destroy(node->lua_engine);

			list_entry = InterlockedPopEntrySList(lsm->head);
		}

		_aligned_free(lsm->head);

		delete lsm;
		lsm = nullptr;
	}

	return lsm;
}

LuaStateManager* 
lua_state_manager_create(IHttpServer* http_server)
{
	LuaStateManager* lsm = new LuaStateManager();

	assert(lsm != nullptr);

	if (lsm)
	{
		lsm->http_server = http_server;
		lsm->head = (PSLIST_HEADER)_aligned_malloc(
			sizeof(SLIST_HEADER), 
			MEMORY_ALLOCATION_ALIGNMENT
		);

		assert(lsm->head != nullptr);

		if (!lsm->head)
		{
			delete lsm;
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

	if (lua_engine)
	{
		if (lsm && lsm->head)
		{
			LuaStateManagerNode* node = (LuaStateManagerNode*)lua_engine_get_list_entry(lua_engine);

			if (node == nullptr)
			{
				node = (LuaStateManagerNode*)_aligned_malloc(
					sizeof(LuaStateManagerNode),
					MEMORY_ALLOCATION_ALIGNMENT
				);

				if (node)
				{
					node->lua_engine = lua_engine;
					lua_engine_set_list_entry(lua_engine, (SLIST_ENTRY*)node);

					assert(lua_engine_get_list_entry(lua_engine) == (SLIST_ENTRY*)node);
					assert(lua_engine == node->lua_engine);

					InterlockedPushEntrySList(lsm->head, &node->item_entry);
				}
				else
				{
					goto destroy;
				}
			}
			else
			{
				InterlockedPushEntrySList(lsm->head, &node->item_entry);
			}
		}
		else
		{
			goto destroy;
		}
	}

	goto finish;

destroy:
	if (lua_engine)
	{
		lua_engine = lua_engine_destroy(lua_engine);
	}
finish:
	return nullptr;
}