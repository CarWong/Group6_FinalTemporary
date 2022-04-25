#include "SimplePlayerControl.h"

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/InputEngine.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Application/Application.h"

#include <iostream>
#include <GLFW/glfw3.h>
#include <GLM/gtc/quaternion.hpp>
#define  GLM_SWIZZLE

SimplePlayerControl::SimplePlayerControl() : IComponent(), _moveSpeed(3.0f)
{
}

SimplePlayerControl::~SimplePlayerControl() = default;

void SimplePlayerControl::setCamera(Gameplay::GameObject::Sptr cam)
{
	cameraAttached = cam;
}

void SimplePlayerControl::Update(float deltaTime)
{
	if (Application::Get().IsFocused) {

		// for when free camera being used (not needed atm)
		if (!InputEngine::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) 
		{
		}
		glm::vec3 _moveVector = glm::vec3(0.0f);

		Gameplay::Physics::RigidBody::Sptr physics = GetGameObject()->Get<Gameplay::Physics::RigidBody>();

		//movement commands for left and right
		if (InputEngine::IsKeyDown(GLFW_KEY_A)) 
		{
			_moveVector = glm::vec3(3.0f, 0.0f, 0.0f);
		}

		if (InputEngine::IsKeyDown(GLFW_KEY_D)) 
		{
			_moveVector = glm::vec3(-3.0f, 0.0f, 0.0f);
		}

		_moveVector.z = 0.0f;
		_moveVector *= deltaTime;

		if (physics != NULL)
		{
			physics->ApplyImpulse(_moveVector);
		}
		else
		{
			GetGameObject()->SetPostion(GetGameObject()->GetPosition() + _moveVector);
		}
	}
}



void SimplePlayerControl::RenderImGui()
{
}

nlohmann::json SimplePlayerControl::ToJson() const
{
	return {};
}

SimplePlayerControl::Sptr SimplePlayerControl::FromJson(const nlohmann::json & blob)
{
	SimplePlayerControl::Sptr result = std::make_shared<SimplePlayerControl>();
	return result;
}
