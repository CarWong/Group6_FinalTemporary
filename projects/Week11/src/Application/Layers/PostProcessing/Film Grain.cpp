#include "Film Grain.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

FilmGrain::FilmGrain() :
	PostProcessingLayer::Effect(),
	_shader(nullptr),
	_amount(0.001)
{
	Name = "Film Grain";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/film_grain.glsl" }
	});
}

FilmGrain::~FilmGrain() = default;

void FilmGrain::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();
	_shader->SetUniform("u_amount", _amount);
}

void FilmGrain::RenderImGui()
{
	LABEL_LEFT(ImGui::SliderFloat, "Amount", &_amount, 0.001f, 1.0f);
}

FilmGrain::Sptr FilmGrain::FromJson(const nlohmann::json & data)
{
	return FilmGrain::Sptr();
}

nlohmann::json FilmGrain::ToJson() const
{
	return nlohmann::json();
}
