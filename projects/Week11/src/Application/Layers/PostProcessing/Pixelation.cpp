#include "Pixelation.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

Pixelation::Pixelation() :
	PostProcessingLayer::Effect(),
	_shader(nullptr),
	_pixels(4096.0)
{
	Name = "Pixelation";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/pixel.glsl" }
	});
}

Pixelation::~Pixelation() = default;

void Pixelation::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	_shader->SetUniform("u_pixels", _pixels);
}

void Pixelation::RenderImGui()
{
	LABEL_LEFT(ImGui::SliderFloat, "Pixelation", &_pixels, 256.0f, 4096.0f);
}

Pixelation::Sptr Pixelation::FromJson(const nlohmann::json & data)
{
	return Pixelation::Sptr();
}

nlohmann::json Pixelation::ToJson() const
{
	return nlohmann::json();
}
