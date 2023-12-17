//#include <gameplay/BulletMovementSystem.h>
//
//
//#include <gameplay/bulletcontrollercomponent.h>
//#include <graphics/transformcomponent.h>
//
//gdpNamespaceBegin
//
//void BulletMovementSystem::Execute(const std::vector<Entity*>& entities, float dt)
//{
//	BulletControllerComponent* controller = nullptr;
//	TransformComponent* transform = nullptr;
//
//	const float speed = 10.0f;
//
//	for (int i = 0; i < entities.size(); i++)
//	{
//		Entity* entity = entities[i];
//		controller = entity->GetComponent<BulletControllerComponent>();
//		transform = entity->GetComponent<TransformComponent>();
//
//
//		if (controller != nullptr && transform != nullptr)
//		{
//			transform->position += controller->direction * speed * dt;
//		}
//	}
//}
//
//gdpNamespaceEnd