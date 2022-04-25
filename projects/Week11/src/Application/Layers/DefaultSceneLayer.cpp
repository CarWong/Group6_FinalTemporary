#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/Textures/Texture2DArray.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/SimplePlayerControl.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
#include "Gameplay/Components/ShipMoveBehaviour.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr shipMesh   = ResourceManager::CreateAsset<MeshResource>("fenrir.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particlesRR.png", 2, 2);

		//Final Textures & Meshes
		MeshResource::Sptr sqrMesh = ResourceManager::CreateAsset<MeshResource>("platform2.obj");
		MeshResource::Sptr mainCharMesh = ResourceManager::CreateAsset<MeshResource>("trashy.obj");
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>("plane.obj");
		
		Texture2D::Sptr platformTex = ResourceManager::CreateAsset<Texture2D>("textures/Platform.png");
		Texture2D::Sptr lavaTex = ResourceManager::CreateAsset<Texture2D>("textures/beans.png");
		Texture2D::Sptr mainCharTex = ResourceManager::CreateAsset<Texture2D>("textures/trashyTEX.png");
		Texture2D::Sptr backgroundTex = ResourceManager::CreateAsset<Texture2D>("textures/backgroundexam.png");
		Texture2D::Sptr winTex = ResourceManager::CreateAsset<Texture2D>("textures/winscreen.png");
		Texture2D::Sptr loseTex = ResourceManager::CreateAsset<Texture2D>("textures/losescreen.png");
		Texture2D::Sptr ballTex = ResourceManager::CreateAsset<Texture2D>("textures/ball.jpg");
		

		//DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/Group6.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			monkeyMaterial->Set("u_Material.NormalMap", normalMapDefault);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon"; 
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f); 
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr polka = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			polka->Name = "Polka";
			polka->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
			polka->Set("u_Material.Specular", solidBlackTex);
			polka->Set("u_Material.NormalMap", normalMapDefault);
			polka->Set("u_Material.EmissiveMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
		}

		Material::Sptr whiteBrick = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			whiteBrick->Name = "White Bricks";
			whiteBrick->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png"));
			whiteBrick->Set("u_Material.Specular", solidGrey);
			whiteBrick->Set("u_Material.NormalMap", ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png"));
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f); 
		}

		// Final Materials

		Material::Sptr platformMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			platformMat->Name = "platform";
			platformMat->Set("u_Material.AlbedoMap", platformTex);
			platformMat->Set("u_Material.Shininess", 0.1f);
			platformMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr lavaMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			lavaMat->Name = "lava";
			lavaMat->Set("u_Material.AlbedoMap", lavaTex);
			lavaMat->Set("u_Material.Shininess", 0.1f);
			lavaMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr mainCharMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			mainCharMat->Name = "main character";
			mainCharMat->Set("u_Material.AlbedoMap", mainCharTex);
			mainCharMat->Set("u_Material.Shininess", 0.1f);
			mainCharMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr backgroundMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			backgroundMat->Name = "background";
			backgroundMat->Set("u_Material.AlbedoMap", backgroundTex);
			backgroundMat->Set("u_Material.Shininess", 0.1f);
			backgroundMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr winMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			winMat->Name = "win";
			winMat->Set("u_Material.AlbedoMap", winTex);
			winMat->Set("u_Material.Shininess", 0.1f);
			winMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr loseMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			loseMat->Name = "lose";
			loseMat->Set("u_Material.AlbedoMap", loseTex);
			loseMat->Set("u_Material.Shininess", 0.1f);
			loseMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr ballMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			ballMat->Name = "ball";
			ballMat->Set("u_Material.AlbedoMap", ballTex);
			ballMat->Set("u_Material.Shininess", 0.1f);
			ballMat->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Create some lights for our scene
		GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		for (int ix = 0; ix < 1; ix++) {
			GameObject::Sptr light = scene->CreateGameObject("Light");
			light->SetPostion(glm::vec3(-5.5f, -1.58f, 4.1f));
			lightParent->AddChild(light);

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
			lightComponent->SetRadius(5.0f);
			lightComponent->SetIntensity(1.0f);

			GameObject::Sptr light2 = scene->CreateGameObject("Light2");
			light2->SetPostion(glm::vec3(0.14f, -3.32f, 3.26f));
			lightParent->AddChild(light2);

			Light::Sptr lightComponent2 = light2->Add<Light>();
			lightComponent2->SetColor(glm::vec3(0.902f, 0.02f,0.02f));
			lightComponent2->SetRadius(10.0f);
			lightComponent2->SetIntensity(1.0f);

			GameObject::Sptr light3 = scene->CreateGameObject("Light3");
			light3->SetPostion(glm::vec3(5.93, -1.86f, 4.76f));
			lightParent->AddChild(light3);

			Light::Sptr lightComponent3 = light3->Add<Light>();
			lightComponent3->SetColor(glm::vec3(1.0f,1.0f,1.0f));
			lightComponent3->SetRadius(5.0f);
			lightComponent3->SetIntensity(1.0f);
		}


		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 0, -4.750, 4 });
			camera->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}



		// Set up all our sample objects
		GameObject::Sptr platform1 = scene->CreateGameObject("platform1");
		{
			platform1->SetPostion(glm::vec3(0.f, 0.0f, 0.0f));
			platform1->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			platform1->SetScale(glm::vec3(10.84f, 1.92f, 1.0f));

			Gameplay::Physics::RigidBody::Sptr physics = platform1->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Static);
			Gameplay::Physics::BoxCollider::Sptr boxCollider = Gameplay::Physics::BoxCollider::Create();
			boxCollider->SetScale(glm::vec3(10.84f, 1.92f, 1.0f));
			physics->AddCollider(boxCollider);

			RenderComponent::Sptr renderer = platform1->Add<RenderComponent>();
			renderer->SetMesh(sqrMesh);
			renderer->SetMaterial(platformMat);
		}

		GameObject::Sptr mainChar = scene->CreateGameObject("main char");
		{
			mainChar->SetPostion(glm::vec3(-5.43f, -0.23f, 2.5f));
			mainChar->SetRotation(glm::vec3(90.0f, 0.0f, -90.0f));
			mainChar->SetScale(glm::vec3(0.7f, 0.7f, 0.7f));

			RenderComponent::Sptr renderer = mainChar->Add<RenderComponent>();
			renderer->SetMesh(mainCharMesh);
			renderer->SetMaterial(mainCharMat);

			Gameplay::Physics::RigidBody::Sptr physics = mainChar->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Dynamic);
			Gameplay::Physics::BoxCollider::Sptr box = Gameplay::Physics::BoxCollider::Create();

			JumpBehaviour::Sptr mainCharJump = mainChar->Add<JumpBehaviour>();

			box->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			box->SetScale(glm::vec3(0.6f, 0.99f, 0.32f));
			physics->AddCollider(box);

			Gameplay::Physics::TriggerVolume::Sptr volume = mainChar->Add<Gameplay::Physics::TriggerVolume>();
			Gameplay::Physics::BoxCollider::Sptr box2 = Gameplay::Physics::BoxCollider::Create();

			box2->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			box2->SetScale(glm::vec3(0.6f, 0.99f, 0.32f));
			volume->AddCollider(box2);

			Gameplay::GameObject::Sptr particlesMC = scene->CreateGameObject("Particles");
			mainChar->AddChild(particlesMC);

			ParticleSystem::Sptr particleManager = particlesMC->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.966f, 0.878f, 0.767f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 10.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 1.5f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.25f, 0.5f };


			particleManager->AddEmitter(emitter);
		}

		GameObject::Sptr ball = scene->CreateGameObject("ball");
		{
			ball->SetPostion(glm::vec3(2.5f, -0.23f, 5.3f));
			ball->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			ball->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));
			
			RenderComponent::Sptr renderer = ball->Add<RenderComponent>();
			renderer->SetMesh(sqrMesh);
			renderer->SetMaterial(ballMat);

			Gameplay::Physics::RigidBody::Sptr physics = ball->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Dynamic);
			Gameplay::Physics::BoxCollider::Sptr box2 = Gameplay::Physics::BoxCollider::Create();
			box2->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));
			physics->AddCollider(box2);

			Gameplay::Physics::TriggerVolume::Sptr volume = ball->Add<Gameplay::Physics::TriggerVolume>();
			Gameplay::Physics::BoxCollider::Sptr box = Gameplay::Physics::BoxCollider::Create();
			
			box->SetPosition(glm::vec3(2.5f, -0.23f, 5.3f));
			box->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));
			volume->AddCollider(box);

			Gameplay::GameObject::Sptr particlesBall = scene->CreateGameObject("Particles");
			mainChar->AddChild(particlesBall);

			ParticleSystem::Sptr particleManager = particlesBall->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(9.0f, 0.0f, 8.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.966f, 0.878f, 0.767f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 10.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 1.5f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.25f, 0.5f };


			particleManager->AddEmitter(emitter);

		}

		GameObject::Sptr backgroundScene = scene->CreateGameObject("background");
		{
			backgroundScene->SetPostion(glm::vec3(0.33f, 3.54f, 0.0f));
			backgroundScene->SetRotation(glm::vec3(-180.0f, 0.0f, 0.0f));
			backgroundScene->SetScale(glm::vec3(16.68f, 15.33f, 16.05f));
			
			RenderComponent::Sptr renderer = backgroundScene->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(backgroundMat);
		}

		GameObject::Sptr winScene = scene->CreateGameObject("win");
		{
			winScene->SetPostion(glm::vec3(0.0f, -2.73f, -4.f));
			winScene->SetRotation(glm::vec3(-180.f, 0.0f, 0.0f));
			winScene->SetScale(glm::vec3(4.49f, 1.0f, 3.44f));

			RenderComponent::Sptr renderer = winScene->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(winMat);
		}

		GameObject::Sptr loseScene = scene->CreateGameObject("lose");
		{
			loseScene->SetPostion(glm::vec3(0.0f, -2.73f, -4.f));
			loseScene->SetRotation(glm::vec3(-180.f, 0.0f, 0.0f));
			loseScene->SetScale(glm::vec3(4.49f, 1.0f, 3.44f));

			RenderComponent::Sptr renderer = loseScene->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(loseMat);
		}
		

		//GameObject::Sptr ship = scene->CreateGameObject("Fenrir");
		//{
		//	// Set position in the scene
		//	ship->SetPostion(glm::vec3(1.5f, 0.0f, 4.0f));
		//	ship->SetScale(glm::vec3(0.1f));

		//	// Create and attach a renderer for the monkey
		//	RenderComponent::Sptr renderer = ship->Add<RenderComponent>();
		//	renderer->SetMesh(shipMesh);
		//	renderer->SetMaterial(grey);

		//	GameObject::Sptr particles = scene->CreateGameObject("Particles");
		//	ship->AddChild(particles);
		//	particles->SetPostion({ 0.0f, -7.0f, 0.0f});

		//	ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
		//	particleManager->Atlas = particleTex;

		//	particleManager->_gravity = glm::vec3(0.0f);

		//	ParticleSystem::ParticleData emitter;
		//	emitter.Type = ParticleType::SphereEmitter;
		//	emitter.TexID = 2;
		//	emitter.Position = glm::vec3(0.0f);
		//	emitter.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
		//	emitter.Lifetime = 1.0f / 50.0f;
		//	emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
		//	emitter.SphereEmitterData.Velocity = 0.5f;
		//	emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
		//	emitter.SphereEmitterData.Radius = 0.5f;
		//	emitter.SphereEmitterData.SizeRange = { 0.5f, 1.0f };

		//	ParticleSystem::ParticleData emitter2;
		//	emitter2.Type = ParticleType::SphereEmitter;
		//	emitter2.TexID = 2;
		//	emitter2.Position = glm::vec3(0.0f);
		//	emitter2.Color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
		//	emitter2.Lifetime = 1.0f / 40.0f;
		//	emitter2.SphereEmitterData.Timer = 1.0f / 40.0f;
		//	emitter2.SphereEmitterData.Velocity = 0.1f;
		//	emitter2.SphereEmitterData.LifeRange = { 0.5f, 1.0f };
		//	emitter2.SphereEmitterData.Radius = 0.25f;
		//	emitter2.SphereEmitterData.SizeRange = { 0.25f, 0.5f };

		//	particleManager->AddEmitter(emitter);
		//	particleManager->AddEmitter(emitter2);

		//	ShipMoveBehaviour::Sptr move = ship->Add<ShipMoveBehaviour>();
		//	move->Center = glm::vec3(0.0f, 0.0f, 4.0f);
		//	move->Speed = 180.0f;
		//	move->Radius = 6.0f;
		//}

		GameObject::Sptr shadowCaster = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster->SetPostion(glm::vec3(-35.4f, -20.47f, 13.020f));
			shadowCaster->SetRotation(glm::vec3(95.f, 55.f, -89.f));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(120.0f), 1.0f, 0.1f, 100.0f));
		}

		/////////////////////////// UI //////////////////////////////		

		GameObject::Sptr particles = scene->CreateGameObject("Particles"); 
		{
			particles->SetPostion({ 2.75f, 3.31f, 4.85f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->Atlas = particleTex;
			particleManager->_gravity = glm::vec3(-3.8f, 0.0f, -2.31f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 0;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(1.f, 0.0f, 0.0f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 10.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 0.5f, 2.0f };
			emitter.SphereEmitterData.Radius = 2.0f;
			emitter.SphereEmitterData.SizeRange = { 1.f, 2.5f };

			particleManager->AddEmitter(emitter);
		}

		if (mainChar->GetPosition().z < 0.f)
		{
			loseScene->SetPostion(glm::vec3(0.0f, -2.73f, 4.f));
			std::cout << "lose";
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
