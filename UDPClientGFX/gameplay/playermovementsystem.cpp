#include <gameplay/playermovementsystem.h>

 
#include <gameplay/playercontrollercomponent.h>
#include <gameplay/bulletcontrollercomponent.h>
#include <graphics/transformcomponent.h>


gdpNamespaceBegin

void PlayerMovementSystem::Execute(const std::vector<Entity*>& entities, float dt)
{
	PlayerControllerComponent* controller = nullptr;
	BulletControllerComponent* bulletController = nullptr;
	TransformComponent* transform = nullptr;

	

	const float speed = 7.f; 
	const float bulletSpeed = 10.0f;

	for (int i = 0; i < entities.size(); i++)
	{
		Entity* entity = entities[i];
		controller = entity->GetComponent<PlayerControllerComponent>();
		bulletController = entity->GetComponent<BulletControllerComponent>(); 
		transform = entity->GetComponent<TransformComponent>();
		//mesh = entity->GetComponent<MeshRendererComponent>();


		if (controller != nullptr && transform != nullptr)
		{
			glm::vec3 direction = glm::vec3(0.f);

			direction.z += controller->moveLeft ? -1.f : 0.f;
			direction.z += controller->moveRight ? 1.f : 0.f;
			direction.x += controller->moveForward ? 1.f : 0.f;
			direction.x += controller->moveBackward ? -1.f : 0.f;
			transform->position += direction * speed * dt;

			//if (controller->shoot)
			//{
			//	Entity* bullet = GetEntityManager().CreateEntity(); 
			//	bullet->AddComponent<MeshRendererComponent>(mesh->vbo, mesh->numTriangles, glm::vec3(0.0f, 1.0f, 0.0f)); 
			//	//bullet->GetComponent< MeshRendererComponent >() = entity->GetComponent<MeshRendererComponent>();//(m_Models[2].Vbo, m_Models[2].NumTriangles, glm::vec3(0.0f, 1.0f, 0.0f));
			//	//printf("hello");
			//	bullet->AddComponent<TransformComponent>(transform->position, glm::vec3(0.5f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
			//	//m_NetworkedEntities.push_back(bullet);

			//}
		}

		if (bulletController != nullptr && transform != nullptr)
		{
			transform->position += bulletController->direction * bulletSpeed * dt;  
		}
	}
}

gdpNamespaceEnd