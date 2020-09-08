#include "shared.h"

/// <summary>
/// The object representing the lua state manager.
/// </summary>
typedef struct _LuaStateManager
{
	unsigned int max_pool_size;

	IHttpServer* http_server;

	SLIST_HEADER* head;
} LuaStateManager;

/// <summary>
/// The node object used to create a linked list of objects.
/// </summary>
typedef struct _LuaStateManagerNode 
{
	SLIST_ENTRY item_entry;
	LuaEngine* lua_engine;
} LuaStateManagerNode;

/// <summary>
/// Invariant check for the lua state manager.
/// </summary>
/// <param name="lsm">The lua state manager object.</param>
/// <returns>True if no assertion is thrown.</returns>
static bool 
lua_state_manager_validate(LuaStateManager* lsm)
{
	assert(lsm != nullptr);
	assert(lsm->head != nullptr);
	assert(lsm->http_server != nullptr);

	return true;
}

/// <summary>
/// Aquires a lua engine object by either fetching an available one from the stack
/// or creating a brand new lua engine.
/// </summary>
/// <param name="lsm">The lua state manager object.</param>
/// <returns>A lua engine object.</returns>
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

			assert(lua_engine->list_entry == list_entry);
			assert(lua_engine == node->lua_engine);
		}
		else
		{
			lua_engine = lua_engine_create(lsm->http_server->GetAppPoolName());
		}
	}

	return lua_engine;
}

/// <summary>
/// Destroys a given lua state manager.
/// </summary>
/// <param name="lsm">The lua state manager to destroy.</param>
/// <returns>A nullptr indicating that the object was removed.</returns>
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

/// <summary>
/// Creates a new lua state manager.
/// </summary>
/// <returns>If successful returns the pointer to the lua state manager,
/// if not, returns nullptr.</returns>
LuaStateManager* 
lua_state_manager_create(IHttpServer* http_server)
{
	LuaStateManager* lsm = new LuaStateManager();

	assert(lsm != nullptr);

	if (lsm)
	{
		lsm->max_pool_size = LUA_STATE_MAX_POOL_SIZE;
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

/// <summary>
/// Releases a given lua engine back into the pool.
/// </summary>
/// <param name="lsm">The corresponding lua state manager to
/// place the lua engine back into.
/// </param>
/// <param name="lua_engine">The lua engine to release.</param>
/// <returns>Returns nullptr to indicate that the lua engine was released.</returns>
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
			if (QueryDepthSList(lsm->head) < lsm->max_pool_size)
			{
				LuaStateManagerNode* node = (LuaStateManagerNode*)lua_engine->list_entry;

				if (node == nullptr)
				{
					node = (LuaStateManagerNode*)_aligned_malloc(
						sizeof(LuaStateManagerNode),
						MEMORY_ALLOCATION_ALIGNMENT
					);

					if (node)
					{
						node->lua_engine = lua_engine;
						lua_engine->list_entry = (SLIST_ENTRY*)node;

						assert(lua_engine->list_entry == (SLIST_ENTRY*)node);
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