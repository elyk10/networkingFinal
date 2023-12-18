#include <engine/engine.h>

#include <sstream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <gameplay/playercontrollercomponent.h>
#include <gameplay/bulletcontrollercomponent.h> 
#include <gameplay/rotatecomponent.h>

#include <graphics/meshrenderercomponent.h>
#include <graphics/transformcomponent.h>
#include <network/netcomponent.h>

#include <gameplay/BulletMovementSystem.h>

#include <system/entitymanager.h>

gdpNamespaceBegin

void _CheckGLError(const char* file, int line)
{
	GLenum err(glGetError());

	while (err != GL_NO_ERROR)
	{
		std::string error;
		switch (err)
		{
		case GL_INVALID_OPERATION:				error = "INVALID_OPERATION";				break;
		case GL_INVALID_ENUM:					error = "INVALID_ENUM";					    break;
		case GL_INVALID_VALUE:					error = "INVALID_VALUE";					break;
		case GL_OUT_OF_MEMORY:					error = "OUT_OF_MEMORY";					break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";    break;
		}
		std::stringstream ss;
		ss << "[" << file << ":" << line << "] " << error;

		printf("OpenGL Error: %s\n", ss.str().c_str());

		err = glGetError();
	}

	return;
}
Engine::Engine()
{
}

Engine::~Engine()
{
}

void Engine::Initialize()
{
	LoadAssets();

	m_GameWorld = new GameWorld();
	m_GameWorld->StartUp();

	m_NetworkManager.Initialize();

	m_LastTime = std::chrono::high_resolution_clock::now();
}

void Engine::Destroy()
{
	m_GameWorld->Shutdown();
	m_NetworkManager.Destroy();
}

void Engine::Resize(int w, int h)
{
	if (w <= 0 || h <= 0) return;
	m_WindowWidth = w;
	m_WindowHeight = h;
	m_WindowRatio = static_cast<float>(w) / static_cast<float>(h);
	glViewport(0, 0, m_WindowWidth, m_WindowHeight);
}

void Engine::Update()
{
	std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> dt = currentTime - m_LastTime;
	m_LastTime = currentTime;

	//m_GameWorld->Update();

	// Handle user Input
	PlayerControllerComponent* controller = m_Player->GetComponent<PlayerControllerComponent>();
	controller->moveBackward = m_Keys['s'];
	controller->moveForward = m_Keys['w'];
	controller->moveLeft = m_Keys['a'];
	controller->moveRight = m_Keys['d'];
	controller->shoot = m_Keys[' '];
	controller->respawn = m_Keys['r'];

	glm::vec3 direction = glm::vec3(0.f); 

	direction.z += controller->moveLeft ? -1.f : 0.f; 
	direction.z += controller->moveRight ? 1.f : 0.f; 
	direction.x += controller->moveForward ? 1.f : 0.f; 
	direction.x += controller->moveBackward ? -1.f : 0.f; 

	if (controller->shoot)
	{
		//printf("hello");
		Entity* bullet = GetEntityManager().CreateEntity();
		bullet->AddComponent<MeshRendererComponent>(m_Models[2].Vbo, m_Models[2].NumTriangles, glm::vec3(0.0f, 1.0f, 0.0f));
		bullet->AddComponent<BulletControllerComponent>();
		bullet->GetComponent<BulletControllerComponent>()->direction = direction; 
		bullet->AddComponent<TransformComponent>(m_Player->GetComponent<TransformComponent>()->position, glm::vec3(0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
		bullet->AddComponent<NetComponent>(true); 
		//m_NetworkedEntities.push_back(bullet); 
		//m_Systems.push_back(bullet->GetComponent)
	}
	
	std::vector<Entity*> entities;
	GetEntityManager().GetEntities(entities);

	for (int i = 0; i < m_Systems.size(); i++)
	{
		m_Systems[i]->Execute(entities, dt.count());
	}

	std::vector<float>x, z;

	for (int i = 0; i < entities.size(); i++)
	{
		Entity* entity = entities[i]; 
		
		BulletControllerComponent* bulletController = nullptr;
		TransformComponent* transform = nullptr;

		bulletController = entity->GetComponent<BulletControllerComponent>();
		transform = entity->GetComponent<TransformComponent>();

		if (bulletController != nullptr && transform != nullptr)
		{
			x.push_back(transform->position.x); 
			z.push_back(transform->position.z); 
		}

	}

	TransformComponent* transform = m_Player->GetComponent<TransformComponent>(); 

	m_NetworkManager.SendBulletPositionsToServer(x, z);
	m_NetworkManager.SendPlayerPositionToServer(transform->position.x, transform->position.z);
	m_NetworkManager.Update();

	for (int i = 0; i < 4; i++)
	{
		TransformComponent* transform = m_NetworkedEntities[i]->GetComponent<TransformComponent>();
		transform->position.x = m_NetworkManager.m_NetworkedPositions[i].x;
		transform->position.z = m_NetworkManager.m_NetworkedPositions[i].z;
	}

	if (m_NetworkManager.m_NetworkedPositions.size() > 4)
	{
		for (int i = 4; i < m_NetworkManager.m_NetworkedPositions.size(); i++)
		{
			std::cout << "I: " << i << " ne: " << m_NetworkedEntities.size() << std::endl;

			if (m_NetworkedEntities.size() > i)
			{
				//printf("hellot");
				TransformComponent* transform = m_NetworkedEntities[i]->GetComponent<TransformComponent>(); 
				transform->position.x = m_NetworkManager.m_NetworkedPositions[i].x; 
				transform->position.z = m_NetworkManager.m_NetworkedPositions[i].z; 

			}
			else
			{

				Entity* bullet = GetEntityManager().CreateEntity();
				bullet->AddComponent<MeshRendererComponent>(m_Models[2].Vbo, m_Models[2].NumTriangles, glm::vec3(0.0f, 0.0f, 1.0f));
				TransformComponent transform(glm::vec3(m_NetworkManager.m_NetworkedPositions[i].x,
					0.0f, m_NetworkManager.m_NetworkedPositions[i].z), glm::vec3(0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f)); 
				bullet->AddComponent<TransformComponent>(transform); 
				//bullet->AddComponent<BulletControllerComponent>();
				//bullet->GetComponent<BulletControllerComponent>()->direction = direction;
				//bullet->AddComponent<TransformComponent>(m_Player->GetComponent<TransformComponent>()->position, glm::vec3(0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
				bullet->AddComponent<NetComponent>(true);
				m_NetworkedEntities.push_back(bullet);
			}
		}
	}
}

void Engine::Render()
{
	glClear(GL_COLOR_BUFFER_BIT);

	const glm::vec3 up(0.f, 1.f, 0.f);
	const glm::vec3 forward(0.f, 0.f, -1.f);

	glm::mat4 projectionMatrix = glm::perspective(
		glm::radians(45.0f),
		((GLfloat)m_WindowWidth) / ((GLfloat)m_WindowHeight),
		0.1f,
		10000.0f
	);
	glUniformMatrix4fv(m_ProjectionMatrixUL, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	CheckGLError();


	TransformComponent* cameraTransform = m_CameraEntity->GetComponent<TransformComponent>();
	glm::vec3 cameraPosition = cameraTransform->position;
	glm::vec3 cameraForward = cameraTransform->orientation * forward;

	glm::vec3 toOrigin = glm::normalize(-cameraPosition);
	glm::mat4 viewMatrix = glm::lookAt(cameraPosition, toOrigin, up);
	//glm::mat4 viewMatrix = glm::lookAt(glm::vec3(-10.f, 0.f, 0.f), glm::vec3(-9.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	glUniformMatrix4fv(m_ViewMatrixUL, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	CheckGLError();

	std::vector<Entity*> entities;
	GetEntityManager().GetEntities(entities);

	for (int i = 0; i < entities.size(); i++)
	{
		Entity* entity = entities[i];
		if (entity->HasComponent<MeshRendererComponent>() && entity->HasComponent<TransformComponent>())
		{
			MeshRendererComponent* renderer = entity->GetComponent<MeshRendererComponent>();
			TransformComponent* transform = entity->GetComponent<TransformComponent>();

			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.f), transform->position);
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.f), transform->scale);
			glm::mat4 rotationMatrix = glm::mat4_cast(transform->orientation);
			glm::mat4 modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;

			glUniformMatrix4fv(m_ModelMatrixUL, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			CheckGLError();

			glUniform3fv(m_ColorUL, 1, glm::value_ptr(renderer->color));
			CheckGLError();

			glBindVertexArray(renderer->vbo);
			CheckGLError();

			glDrawElements(GL_TRIANGLES, renderer->numTriangles * 3, GL_UNSIGNED_INT, (GLvoid*)0);
			CheckGLError();
		}
	}


	glutSwapBuffers();
}

void Engine::PressKey(unsigned char key)
{
	printf("%c down!\n", key);
	m_Keys[key] = true;
}

void Engine::ReleaseKey(unsigned char key)
{
	printf("%c up!\n", key);
	m_Keys[key] = false;
}

void Engine::PressSpecialKey(int key)
{
}

void Engine::ReleaseSpecialKey(int key)
{
}

void Engine::MouseMotion(int x, int y)
{
}

void Engine::MouseDrag(int x, int y)
{
}

void Engine::MouseButton(int button, int state)
{
}

void Engine::LoadAssets()
{
	// Load shader
	ShaderProgram simpleShader(
		"assets/shaders/SimpleShader.vertex.glsl",
		"assets/shaders/SimpleShader.fragment.glsl"
	);

	m_ShaderPrograms.push_back(simpleShader);
	CheckGLError();
	
	//
	// Move this:
	glUseProgram(simpleShader.id);
	CheckGLError();
	m_ProjectionMatrixUL = glGetUniformLocation(simpleShader.id, "ProjectionMatrix");
	m_ViewMatrixUL = glGetUniformLocation(simpleShader.id, "ViewMatrix");
	m_ModelMatrixUL = glGetUniformLocation(simpleShader.id, "ModelMatrix");
	m_ColorUL = glGetUniformLocation(simpleShader.id, "Color");
	CheckGLError();

	//
	// Meshes
	Model cone("assets/models/cone.obj");
	Model cube("assets/models/cube.obj");
	Model cylinder("assets/models/cylinder.obj");
	Model sphere("assets/models/sphere.obj");
	m_Models.push_back(cone);
	m_Models.push_back(cube);
	m_Models.push_back(cylinder);
	m_Models.push_back(sphere);

	//
	// Textures
	
	//
	// Entities
	const glm::vec3 origin(0.f);
	const glm::vec3 unscaled(1.f);
	const glm::quat identity(1.f, 0.f, 0.f, 0.f);

	//
	// Player #1
	m_Player = GetEntityManager().CreateEntity();
	m_Player->AddComponent<MeshRendererComponent>(sphere.Vbo, sphere.NumTriangles, glm::vec3(1.f, 0.f, 0.f));
	m_Player->AddComponent<TransformComponent>(origin, unscaled, identity);
	m_Player->AddComponent<PlayerControllerComponent>();
	// TOOD: Add network component

	for (int i = 0; i < 4; i++)
	{
		Entity* entity = GetEntityManager().CreateEntity();
		entity->AddComponent<MeshRendererComponent>(sphere.Vbo, sphere.NumTriangles, glm::vec3(0.3f, 0.3f, 0.3f));
		entity->AddComponent<TransformComponent>(origin, unscaled, identity);
		m_NetworkedEntities.push_back(entity);
	}

	

	//
	// Camera
	m_CameraEntity = GetEntityManager().CreateEntity();
	glm::quat rotation = identity * glm::vec3(0.f, -1.f, -0.1f);
	m_CameraEntity->AddComponent<TransformComponent>(glm::vec3(-20.f, 50.f, 0.f), unscaled, rotation);
}

void Engine::AddSystem(System* system)
{
	m_Systems.push_back(system);
}
gdpNamespaceEnd