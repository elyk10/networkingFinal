#pragma once
#include <common.h>

#include <glm/glm.hpp>

#include <system/component.h>

gdpNamespaceBegin
struct BulletControllerComponent : public Component
{
public:
	BulletControllerComponent() {}

	glm::vec3 direction = glm::vec3(0.0f);
};
gdpNamespaceEnd 