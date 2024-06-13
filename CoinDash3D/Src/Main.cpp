#include <cstdint>
#include <Windows.h>

#if defined(DEBUG_MODE) || defined(RELEASE_MODE) || defined(DEVELOPMENT_MODE)
#include <crtdbg.h>
#endif

#include "CrashModule.h"
#include "GameModule.h"
#include "GLTFLoader.h"
#include "PlatformModule.h"
#include "RenderModule.h"

#include "Camera.h"
#include "GLTFLoader.h"
#include "GeometryRenderer2D.h"
#include "GeometryRenderer3D.h"
#include "SkinnedMesh.h"
#include "StaticMesh.h"
#include "TextRenderer.h"
#include "TTFont.h"
#include "Shader.h"
#include "GLTFLoader.h"
#include "CrossFadeController.h"
#include "BaseColorMap.h"
#include "TileColorMap.h"

#include <imgui.h>

int32_t WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int32_t nCmdShow)
{
#if defined(DEBUG_MODE) || defined(RELEASE_MODE) || defined(DEVELOPMENT_MODE)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	PlatformModule::WindowConstructParams windowParam{ L"CoinDash3D", 100, 100, 800, 600, false, false };

	CrashModule::Init();
	PlatformModule::Init(windowParam);
	RenderModule::Init(PlatformModule::GetWindowHandle());
	GameModule::Init();

	PlatformModule::SetEndLoopCallback([&]() { RenderModule::Uninit(); });
	
	GeometryRenderer2D* renderer2d = RenderModule::CreateResource<GeometryRenderer2D>();
	GeometryRenderer3D* renderer3d = RenderModule::CreateResource<GeometryRenderer3D>();
	TextRenderer* textRenderer = RenderModule::CreateResource<TextRenderer>();
	TTFont* font = RenderModule::CreateResource<TTFont>("Resource/Font/SeoulNamsanEB.ttf", 0, 0x127, 32.0f);
	BaseColorMap* baseColorMap = RenderModule::CreateResource<BaseColorMap>(BaseColorMap::ESize::Size_1024x1024, Vec4f(1.0f, 0.5f, 0.0f, 1.0f));
	TileColorMap* tileColorMap = RenderModule::CreateResource<TileColorMap>(
		TileColorMap::ESize::Size_1024x1024, 
		TileColorMap::ESize::Size_16x16,
		Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
		Vec4f(1.0f, 0.0f, 0.0f, 1.0f)
	);
	Camera* camera = GameModule::CreateEntity<Camera>();

	Mat4x4 screenOrtho = RenderModule::GetScreenOrtho();
	renderer2d->SetOrtho(screenOrtho);
	textRenderer->SetOrtho(screenOrtho);

	cgltf_data* data = GLTFLoader::Load("Resource/Model/test11.glb");
	std::vector<GLTFLoader::MeshResource> meshResources = GLTFLoader::LoadMeshResources(data);
	GLTFLoader::Free(data);

	std::vector<StaticMesh*> meshes;
	for (const auto& meshResource : meshResources)
	{
		std::vector<StaticMesh::Vertex> vertices(meshResource.positions.size());
		std::vector<uint32_t> indices = meshResource.indices;

		for (uint32_t index = 0; index < vertices.size(); ++index)
		{
			vertices[index].position = meshResource.positions[index];
			vertices[index].normal = meshResource.normals[index];
			vertices[index].tangent = meshResource.tangents[index];
			vertices[index].texcoord = meshResource.texcoords[index];
		}

		meshes.push_back(RenderModule::CreateResource<StaticMesh>(vertices, indices));
	}

	Shader* shader = RenderModule::CreateResource<Shader>("Resource/Shader/StaticMesh.vert", "Resource/Shader/Mesh.frag");

	PlatformModule::RunLoop(
		[&](float deltaSeconds)
		{
			camera->Tick(deltaSeconds);

			renderer3d->SetView(camera->GetView());
			renderer3d->SetProjection(camera->GetProjection());

			RenderModule::BeginFrame(0.0f, 0.0f, 0.0f, 1.0f);

			renderer3d->DrawGrid3D(Vec3f(100.0f, 100.0f, 100.0f), 1.0f);

			shader->Bind();
			{
				tileColorMap->Active(0);

				shader->SetUniform("world", Mat4x4::Identity());
				shader->SetUniform("view", camera->GetView());
				shader->SetUniform("projection", camera->GetProjection());

				for (const auto& mesh : meshes)
				{
					mesh->Bind();
					RenderModule::ExecuteDrawIndex(mesh->GetIndexCount(), EDrawMode::TRIANGLES);
					mesh->Unbind();
				}
			}
			shader->Unbind();

			RenderModule::EndFrame();
		}
	);

	GameModule::Uninit();
	PlatformModule::Uninit();
	CrashModule::Uninit();
	return 0;
}