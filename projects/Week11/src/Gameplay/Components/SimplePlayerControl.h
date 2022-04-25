#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"

struct GLFWwindow;

/// Allow player to move with WASD
class SimplePlayerControl : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<SimplePlayerControl> Sptr;

	//constructor and destructor
	SimplePlayerControl();
	virtual ~SimplePlayerControl();

	//camera to follow player if needed
	void setCamera(Gameplay::GameObject::Sptr cam);

	//update function
	virtual void Update(float deltaTime) override;

public:

	//debug options
	virtual void RenderImGui() override;
	MAKE_TYPENAME(SimplePlayerControl);
	virtual nlohmann::json ToJson() const override;
	static SimplePlayerControl::Sptr FromJson(const nlohmann::json& blob);

protected:
	//set limit to movement and attatch camera if needed
	float _moveSpeed;
	Gameplay::GameObject::Sptr cameraAttached;
};