#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Framebuffer.h"

class BloomEffect : public PostProcessingLayer::Effect {
public:
	MAKE_PTRS(BloomEffect);

	BloomEffect();
	virtual ~BloomEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	//taken from IResource
	BloomEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _HoriBlurShader;
	ShaderProgram::Sptr _VertBlurShader;
	ShaderProgram::Sptr _BrightShader;
	ShaderProgram::Sptr _ComboShader;
	Framebuffer::Sptr _Horizontal;
	Framebuffer::Sptr _Vertical;
	VertexArrayObject::Sptr _quadVAO;
	float _weights[5] = { 0.22f, 0.19f, 0.12, 0.05, 0.03 };
	float _radius;
	float _threshold;
};
