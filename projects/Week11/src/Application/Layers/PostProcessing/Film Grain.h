#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"

class FilmGrain : public PostProcessingLayer::Effect
{
public:
	MAKE_PTRS(FilmGrain);

	FilmGrain();
	virtual ~FilmGrain();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	//taken from IResource
	FilmGrain::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;
	float				_amount;
};