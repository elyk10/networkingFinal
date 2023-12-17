#include <gameplay/gameworld.h>

#include <graphics/meshrenderercomponent.h>
#include <graphics/transformcomponent.h>

#include <network/netcomponent.h>

#include <system/entitymanager.h>

gdpNamespaceBegin

GameWorld::GameWorld()
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::StartUp()
{
	// Create our player object
	Entity* player = GetEntityManager().CreateEntity();
	player->AddComponent<MeshRendererComponent>();
	player->AddComponent<TransformComponent>();
	player->AddComponent<NetComponent>(true);
}

void GameWorld::Shutdown()
{
}

void GameWorld::Update()
{
}

gdpNamespaceEnd