#include "Bloom Effect.h"
#include "Application/Application.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

#include <GLM/glm.hpp>

BloomEffect::BloomEffect() :
	PostProcessingLayer::Effect(),
	_HoriBlurShader(nullptr),
	_VertBlurShader(nullptr),
	_BrightShader(nullptr),
	_ComboShader(nullptr),
	_threshold(1),
	_radius(1)
{
	Name = "Bloom Effect";
	_format = RenderTargetType::ColorRgb8;

	_HoriBlurShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/horizontal_blur.glsl" }
	});
	_VertBlurShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/vertical_blur.glsl" }
	});
	_BrightShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/bright.glsl" }
	});
	_ComboShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/screen_overlay.glsl" }
	});

	//make needed framebuffers
	Application& app = Application::Get();
	const glm::uvec4& viewport = app.GetPrimaryViewport();

	FramebufferDescriptor fboDesc = FramebufferDescriptor();
	fboDesc.Width = viewport.z / 4;
	fboDesc.Height = viewport.w / 4;
	fboDesc.RenderTargets[RenderTargetAttachment::Color0] = RenderTargetDescriptor(_format);

	_Horizontal = std::make_shared<Framebuffer>(fboDesc);
	_Vertical = std::make_shared<Framebuffer>(fboDesc);

	// We need a mesh for drawing fullscreen quads
	glm::vec2 positions[6] = {
		{ -1.0f,  1.0f }, { -1.0f, -1.0f }, { 1.0f, 1.0f },
		{ -1.0f, -1.0f }, {  1.0f, -1.0f }, { 1.0f, 1.0f }
	};

	VertexBuffer::Sptr vbo = std::make_shared<VertexBuffer>();
	vbo->LoadData(positions, 6);

	_quadVAO = VertexArrayObject::Create();
	_quadVAO->AddVertexBuffer(vbo, {
		BufferAttribute(0, 2, AttributeType::Float, sizeof(glm::vec2), 0, AttribUsage::Position)
		});
}

BloomEffect::~BloomEffect() = default;

void BloomEffect::Apply(const Framebuffer::Sptr & gBuffer)
{
	_quadVAO->Bind();

	_BrightShader->Bind();
	_BrightShader->SetUniform("u_threshold", _threshold);

	// Bind the FBO and make sure we're rendering to the whole thing
	_Vertical->Bind();
	glViewport(0, 0, _Vertical->GetWidth(), _Vertical->GetHeight());

	// Bind color 0 from previous pass to texture slot 0 so our effects can access
	gBuffer->BindAttachment(RenderTargetAttachment::Color0, 0);

	// Apply the effect and render the fullscreen quad
	_quadVAO->Draw();

	// Unbind output and set it as input for next pass
	_Vertical->Unbind();

	for (int i = 0; i < 4; i++)
	{
		_HoriBlurShader->Bind();
		glViewport(0, 0, _Horizontal->GetWidth(), _Horizontal->GetHeight());
		_Vertical->BindAttachment(RenderTargetAttachment::Color0, 0);
		_Horizontal->Bind();

		_HoriBlurShader->SetUniform("u_step", _radius / _output->GetWidth());
		_HoriBlurShader->SetUniform("u_weights", _weights);

		_quadVAO->Draw();
		_Horizontal->Unbind();

		//repeat for vertical
		_VertBlurShader->Bind();
		glViewport(0, 0, _Vertical->GetWidth(), _Vertical->GetHeight());
		_Horizontal->BindAttachment(RenderTargetAttachment::Color0, 0);
		_Vertical->Bind();

		_VertBlurShader->SetUniform("u_step", _radius / _output->GetHeight());
		_VertBlurShader->SetUniform("u_weights", _weights);

		_quadVAO->Draw();
		_Vertical->Unbind();
	}

	_ComboShader->Bind();
	_output->Bind();
	gBuffer->BindAttachment(RenderTargetAttachment::Color0, 0);
	_Vertical->BindAttachment(RenderTargetAttachment::Color0, 1);

	_quadVAO->Unbind();
	glViewport(0, 0, _output->GetWidth(), _output->GetHeight());
}

void BloomEffect::RenderImGui()
{
	LABEL_LEFT(ImGui::SliderFloat, "Threshold", &_threshold, 0.1f, 10.0f);
	LABEL_LEFT(ImGui::SliderFloat, "Radius", &_radius, 0.1f, 10.0f);
}

BloomEffect::Sptr BloomEffect::FromJson(const nlohmann::json & data)
{
	return BloomEffect::Sptr();
}

nlohmann::json BloomEffect::ToJson() const
{
	return nlohmann::json();
}
