#pragma warning(push)
#pragma warning(disable: 26495)
#pragma warning(disable: 26819)
#pragma warning(disable: 26451)
#pragma warning(disable: 6031)

#include <glad/glad.h>

#include "Assertion.h"
#include "Atlas2D.h"
#include "Camera2D.h"
#include "ITexture.h"
#include "RenderManager2D.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "TTFont.h"
#include "VertexBuffer.h"

RenderManager2D& RenderManager2D::Get()
{
	static RenderManager2D instance;
	return instance;
}

void RenderManager2D::Startup()
{
	app_ = IApp::Get();

	uint32_t byteSize = static_cast<uint32_t>(Vertex::GetStride() * vertices_.size());
	VertexBuffer::Usage usage = VertexBuffer::Usage::DYNAMIC;
	vertexBuffer_ = ResourceManager::Get().Create<VertexBuffer>(byteSize, usage);

	GL_CHECK(glGenVertexArrays(1, &vertexArrayObject_));
	GL_CHECK(glBindVertexArray(vertexArrayObject_));
	{
		vertexBuffer_->Bind();

		GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, Vertex::GetStride(), (void*)(offsetof(Vertex, position))));
		GL_CHECK(glEnableVertexAttribArray(0));

		GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Vertex::GetStride(), (void*)(offsetof(Vertex, uv))));
		GL_CHECK(glEnableVertexAttribArray(1));

		GL_CHECK(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Vertex::GetStride(), (void*)(offsetof(Vertex, color))));
		GL_CHECK(glEnableVertexAttribArray(2));

		GL_CHECK(glVertexAttribIPointer(3, 1, GL_INT, Vertex::GetStride(), (void*)(offsetof(Vertex, unit))));
		GL_CHECK(glEnableVertexAttribArray(3));

		vertexBuffer_->Unbind();
	}
	GL_CHECK(glBindVertexArray(0));

	Shader* geometry2D = ResourceManager::Get().Create<Shader>("GameMaker/Shader/Geometry2D.vert", "GameMaker/Shader/Geometry2D.frag");
	Shader* sprite2D = ResourceManager::Get().Create<Shader>("GameMaker/Shader/Sprite2D.vert", "GameMaker/Shader/Sprite2D.frag");
	Shader* string2D = ResourceManager::Get().Create<Shader>("GameMaker/Shader/String2D.vert", "GameMaker/Shader/String2D.frag");

	shaders_.insert({ RenderCommand::Type::GEOMETRY, geometry2D });
	shaders_.insert({ RenderCommand::Type::SPRITE,   sprite2D });
	shaders_.insert({ RenderCommand::Type::STRING,   string2D });
}

void RenderManager2D::Shutdown()
{
	ResourceManager::Get().Destroy(vertexBuffer_);
	vertexBuffer_ = nullptr;

	for (auto& shader : shaders_)
	{
		ResourceManager::Get().Destroy(shader.second);
	}

	GL_CHECK(glDeleteVertexArrays(1, &vertexArrayObject_));

	app_ = nullptr;
}

void RenderManager2D::Begin(const Camera2D* camera2D)
{
	CHECK(!bIsBegin_);

	GameMath::Mat4x4 ortho;
	if (!camera2D)
	{
		float w = 0.0f;
		float h = 0.0f;
		static const float nearZ = -1.0f;
		static const float farZ = 1.0f;
		app_->GetScreenSize<float>(w, h);

		ortho = GameMath::Mat4x4::Ortho(-w * 0.5f, +w * 0.5f, -h * 0.5f, +h * 0.5f, nearZ, farZ);
	}
	else
	{
		ortho = camera2D->GetOrtho();
	}

	for (auto& shader : shaders_)
	{
		Shader* shaderPtr = shader.second;

		shaderPtr->Bind();
		{
			shaderPtr->SetUniform("ortho", ortho);
		}
		shaderPtr->Unbind();
	}

	GLboolean originEnableDepth;
	GL_CHECK(glGetBooleanv(GL_DEPTH_TEST, &originEnableDepth));

	GLboolean originEnableCull;
	GL_CHECK(glGetBooleanv(GL_CULL_FACE, &originEnableCull));

	originEnableDepth_ = static_cast<bool>(originEnableDepth);
	originEnableCull_ = static_cast<bool>(originEnableCull);

	app_->SetDepthMode(false);
	app_->SetCullFaceMode(false);

	bIsBegin_ = true;
}

void RenderManager2D::End()
{
	CHECK(bIsBegin_);

	Flush();

	app_->SetCullFaceMode(originEnableCull_);
	app_->SetDepthMode(originEnableDepth_);

	bIsBegin_ = false;
}

void RenderManager2D::DrawPoint(const GameMath::Vec2f& point, const GameMath::Vec4f& color, float pointSize)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w = pointSize * 0.5f;
	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w, -w),
		GameMath::Vec2f(+w, +w),
		GameMath::Vec2f(-w, +w),
		GameMath::Vec2f(-w, -w),
		GameMath::Vec2f(+w, -w),
		GameMath::Vec2f(+w, +w),
	};

	for (auto& vertex : vertices)
	{
		vertex += (point + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawLine(const GameMath::Vec2f& startPos, const GameMath::Vec2f& endPos, const GameMath::Vec4f& color)
{
	static const uint32_t MAX_VERTEX_SIZE = 2;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		startPos + GameMath::Vec2f(0.375f, 0.375f),
		endPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawLine(const GameMath::Vec2f& startPos, const GameMath::Vec4f& startColor, const GameMath::Vec2f& endPos, const GameMath::Vec4f& endColor)
{
	static const uint32_t MAX_VERTEX_SIZE = 2;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		startPos + GameMath::Vec2f(0.375f, 0.375f),
		  endPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	std::array<GameMath::Vec4f, MAX_VERTEX_SIZE> colors =
	{
		startColor,
		endColor,
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = colors[index];
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = colors[index];
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawTriangle(const GameMath::Vec2f& fromPos, const GameMath::Vec2f& byPos, const GameMath::Vec2f& toPos, const GameMath::Vec4f& color)
{
	static const uint32_t MAX_VERTEX_SIZE = 3;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		fromPos + GameMath::Vec2f(0.375f, 0.375f),
		  byPos + GameMath::Vec2f(0.375f, 0.375f),
		  toPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawTriangle(const GameMath::Vec2f& fromPos, const GameMath::Vec4f& fromColor, const GameMath::Vec2f& byPos, const GameMath::Vec4f& byColor, const GameMath::Vec2f& toPos, const GameMath::Vec4f& toColor)
{
	static const uint32_t MAX_VERTEX_SIZE = 3;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		fromPos + GameMath::Vec2f(0.375f, 0.375f),
		  byPos + GameMath::Vec2f(0.375f, 0.375f),
		  toPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	std::array<GameMath::Vec4f, MAX_VERTEX_SIZE> colors =
	{
		fromColor,
		byColor,
		toColor,
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = colors[index];
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = colors[index];
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawTriangleWireframe(const GameMath::Vec2f& fromPos, const GameMath::Vec2f& byPos, const GameMath::Vec2f& toPos, const GameMath::Vec4f& color)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		fromPos + GameMath::Vec2f(0.375f, 0.375f),   byPos + GameMath::Vec2f(0.375f, 0.375f),
		byPos + GameMath::Vec2f(0.375f, 0.375f),   toPos + GameMath::Vec2f(0.375f, 0.375f),
		toPos + GameMath::Vec2f(0.375f, 0.375f), fromPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawTriangleWireframe(const GameMath::Vec2f& fromPos, const GameMath::Vec4f& fromColor, const GameMath::Vec2f& byPos, const GameMath::Vec4f& byColor, const GameMath::Vec2f& toPos, const GameMath::Vec4f& toColor)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		fromPos + GameMath::Vec2f(0.375f, 0.375f),   byPos + GameMath::Vec2f(0.375f, 0.375f),
		byPos + GameMath::Vec2f(0.375f, 0.375f),   toPos + GameMath::Vec2f(0.375f, 0.375f),
		toPos + GameMath::Vec2f(0.375f, 0.375f), fromPos + GameMath::Vec2f(0.375f, 0.375f),
	};

	std::array<GameMath::Vec4f, MAX_VERTEX_SIZE> colors =
	{
		fromColor, byColor,
		byColor,   toColor,
		toColor,   fromColor,
	};

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = colors[index];
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = colors[index];
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawRect(const GameMath::Vec2f& center, float w, float h, const GameMath::Vec4f& color, float rotate)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, +h2),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawRectWireframe(const GameMath::Vec2f& center, float w, float h, const GameMath::Vec4f& color, float rotate)
{
	static const uint32_t MAX_VERTEX_SIZE = 8;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2), GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, -h2), GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(+w2, +h2), GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, +h2), GameMath::Vec2f(-w2, -h2),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate), 
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawRoundRect(const GameMath::Vec2f& center, float w, float h, float side, const GameMath::Vec4f& color, float rotate)
{
	static const uint32_t MAX_VERTEX_SIZE = 252;
	static const uint32_t MAX_SLICE_SIZE = 20;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;
	side = GameMath::Min<float>(side, GameMath::Min<float>(h2, h2));

	uint32_t vertexCount = 0;
	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices;

	auto calculateBezierCurve = [&](const GameMath::Vec2f& startPos, const GameMath::Vec2f& endPos, const GameMath::Vec2f& controlPos, uint32_t sliceCount)
		{
			for (int32_t slice = 0; slice < sliceCount; ++slice)
			{
				float t0 = static_cast<float>(slice + 0) / static_cast<float>(sliceCount);
				float t1 = static_cast<float>(slice + 1) / static_cast<float>(sliceCount);

				vertices[vertexCount + 0] = GameMath::Vec2f();
				vertices[vertexCount + 1] = GameMath::Vec2f::Bezier(startPos, endPos, controlPos, t0);
				vertices[vertexCount + 2] = GameMath::Vec2f::Bezier(startPos, endPos, controlPos, t1);

				vertexCount += 3;
			}
		};

	GameMath::Vec2f control = GameMath::Vec2f(-w2, +h2);
	GameMath::Vec2f start = control + GameMath::Vec2f(+side, 0.0f);
	GameMath::Vec2f end = control + GameMath::Vec2f(0.0f, -side);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f();
	vertices[vertexCount + 1] = GameMath::Vec2f(-w2, +h2) + GameMath::Vec2f(0.0f, -side);
	vertices[vertexCount + 2] = GameMath::Vec2f(-w2, -h2) + GameMath::Vec2f(0.0f, +side);
	vertexCount += 3;

	control = GameMath::Vec2f(-w2, -h2);
	start = control + GameMath::Vec2f(0.0f, +side);
	end = control + GameMath::Vec2f(+side, 0.0f);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f();
	vertices[vertexCount + 1] = GameMath::Vec2f(-w2, -h2) + GameMath::Vec2f(+side, 0.0f);
	vertices[vertexCount + 2] = GameMath::Vec2f(+w2, -h2) + GameMath::Vec2f(-side, 0.0f);
	vertexCount += 3;

	control = GameMath::Vec2f(+w2, -h2);
	start = control + GameMath::Vec2f(-side, 0.0f);
	end = control + GameMath::Vec2f(0.0f, +side);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f();
	vertices[vertexCount + 1] = GameMath::Vec2f(+w2, -h2) + GameMath::Vec2f(0.0f, +side);
	vertices[vertexCount + 2] = GameMath::Vec2f(+w2, +h2) + GameMath::Vec2f(0.0f, -side);
	vertexCount += 3;

	control = GameMath::Vec2f(+w2, +h2);
	start = control + GameMath::Vec2f(0.0f, -side);
	end = control + GameMath::Vec2f(-side, 0.0f);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f();
	vertices[vertexCount + 1] = GameMath::Vec2f(+w2, +h2) + GameMath::Vec2f(-side, 0.0f);
	vertices[vertexCount + 2] = GameMath::Vec2f(-w2, +h2) + GameMath::Vec2f(+side, 0.0f);
	vertexCount += 3;

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (uint32_t index = 0; index < vertexCount; index += 3)
	{
		vertices[index + 1] = rotateMat * vertices[index + 1];
		vertices[index + 2] = rotateMat * vertices[index + 2];

		vertices[index + 0] += (center + GameMath::Vec2f(0.375f, 0.375f));
		vertices[index + 1] += (center + GameMath::Vec2f(0.375f, 0.375f));
		vertices[index + 2] += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawRoundRectWireframe(const GameMath::Vec2f& center, float w, float h, float side, const GameMath::Vec4f& color, float rotate)
{
	static const uint32_t MAX_VERTEX_SIZE = 168;
	static const uint32_t MAX_SLICE_SIZE = 20;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;
	side = GameMath::Min<float>(side, GameMath::Min<float>(h2, h2));

	uint32_t vertexCount = 0;
	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices;

	auto calculateBezierCurve = [&](const GameMath::Vec2f& startPos, const GameMath::Vec2f& endPos, const GameMath::Vec2f& controlPos, uint32_t sliceCount)
		{
			for (int32_t slice = 0; slice < sliceCount; ++slice)
			{
				float t0 = static_cast<float>(slice + 0) / static_cast<float>(sliceCount);
				float t1 = static_cast<float>(slice + 1) / static_cast<float>(sliceCount);

				vertices[vertexCount + 0] = GameMath::Vec2f::Bezier(startPos, endPos, controlPos, t0);
				vertices[vertexCount + 1] = GameMath::Vec2f::Bezier(startPos, endPos, controlPos, t1);
				vertexCount += 2;
			}
		};

	GameMath::Vec2f control = GameMath::Vec2f(-w2, +h2);
	GameMath::Vec2f start = control + GameMath::Vec2f(+side, 0.0f);
	GameMath::Vec2f end = control + GameMath::Vec2f(0.0f, -side);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f(-w2, +h2) + GameMath::Vec2f(0.0f, -side);
	vertices[vertexCount + 1] = GameMath::Vec2f(-w2, -h2) + GameMath::Vec2f(0.0f, +side);
	vertexCount += 2;

	control = GameMath::Vec2f(-w2, -h2);
	start = control + GameMath::Vec2f(0.0f, +side);
	end = control + GameMath::Vec2f(+side, 0.0f);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f(-w2, -h2) + GameMath::Vec2f(+side, 0.0f);
	vertices[vertexCount + 1] = GameMath::Vec2f(+w2, -h2) + GameMath::Vec2f(-side, 0.0f);
	vertexCount += 2;

	control = GameMath::Vec2f(+w2, -h2);
	start = control + GameMath::Vec2f(-side, 0.0f);
	end = control + GameMath::Vec2f(0.0f, +side);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f(+w2, -h2) + GameMath::Vec2f(0.0f, +side);
	vertices[vertexCount + 1] = GameMath::Vec2f(+w2, +h2) + GameMath::Vec2f(0.0f, -side);
	vertexCount += 2;

	control = GameMath::Vec2f(+w2, +h2);
	start = control + GameMath::Vec2f(0.0f, -side);
	end = control + GameMath::Vec2f(-side, 0.0f);
	calculateBezierCurve(start, end, control, MAX_SLICE_SIZE);

	vertices[vertexCount + 0] = GameMath::Vec2f(+w2, +h2) + GameMath::Vec2f(-side, 0.0f);
	vertices[vertexCount + 1] = GameMath::Vec2f(-w2, +h2) + GameMath::Vec2f(+side, 0.0f);
	vertexCount += 2;

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (uint32_t index = 0; index < vertexCount; index += 2)
	{
		vertices[index + 0] = rotateMat * vertices[index + 0];
		vertices[index + 1] = rotateMat * vertices[index + 1];

		vertices[index + 0] += (center + GameMath::Vec2f(0.375f, 0.375f));
		vertices[index + 1] += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawCircle(const GameMath::Vec2f& center, float radius, const GameMath::Vec4f& color)
{
	static const uint32_t MAX_VERTEX_SIZE = 300;
	static const uint32_t MAX_SLICE_SIZE = 100;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	uint32_t vertexCount = 0;
	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices;

	for (int32_t slice = 0; slice < MAX_SLICE_SIZE; ++slice)
	{
		float radian0 = (static_cast<float>(slice + 0) * GameMath::TWO_PI) / static_cast<float>(MAX_SLICE_SIZE);
		float radian1 = (static_cast<float>(slice + 1) * GameMath::TWO_PI) / static_cast<float>(MAX_SLICE_SIZE);

		vertices[vertexCount + 0] = center + GameMath::Vec2f(0.375f, 0.375f);
		vertices[vertexCount + 1] = center + GameMath::Vec2f(radius * GameMath::Cos(radian0), radius * GameMath::Sin(radian0)) + GameMath::Vec2f(0.375f, 0.375f);
		vertices[vertexCount + 2] = center + GameMath::Vec2f(radius * GameMath::Cos(radian1), radius * GameMath::Sin(radian1)) + GameMath::Vec2f(0.375f, 0.375f);

		vertexCount += 3;
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawCircleWireframe(const GameMath::Vec2f& center, float radius, const GameMath::Vec4f& color)
{
	static const uint32_t MAX_VERTEX_SIZE = 200;
	static const uint32_t MAX_SLICE_SIZE = 100;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	uint32_t vertexCount = 0;
	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices;

	for (int32_t slice = 0; slice < MAX_SLICE_SIZE; ++slice)
	{
		float radian0 = (static_cast<float>(slice + 0) * GameMath::TWO_PI) / static_cast<float>(MAX_SLICE_SIZE);
		float radian1 = (static_cast<float>(slice + 1) * GameMath::TWO_PI) / static_cast<float>(MAX_SLICE_SIZE);

		vertices[vertexCount + 0] = center + GameMath::Vec2f(radius * GameMath::Cos(radian0), radius * GameMath::Sin(radian0)) + GameMath::Vec2f(0.375f, 0.375f);
		vertices[vertexCount + 1] = center + GameMath::Vec2f(radius * GameMath::Cos(radian1), radius * GameMath::Sin(radian1)) + GameMath::Vec2f(0.375f, 0.375f);

		vertexCount += 2;
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::LINES && prevCommand.type == RenderCommand::Type::GEOMETRY)
		{
			uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
			prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

			for (uint32_t index = 0; index < vertices.size(); ++index)
			{
				vertices_[startVertexIndex + index].position = vertices[index];
				vertices_[startVertexIndex + index].color = color;
			}

			return;
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	RenderCommand command;
	command.drawMode = DrawMode::LINES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::GEOMETRY;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].color = color;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawSprite(ITexture* texture, const GameMath::Vec2f& center, float w, float h, float rotate, bool bFlipH, bool bFlipV)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, +h2),
	};

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> uvs =
	{
		GameMath::Vec2f(0.0f, 1.0f),
		GameMath::Vec2f(1.0f, 0.0f),
		GameMath::Vec2f(0.0f, 0.0f),
		GameMath::Vec2f(0.0f, 1.0f),
		GameMath::Vec2f(1.0f, 1.0f),
		GameMath::Vec2f(1.0f, 0.0f),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex.x = bFlipH ? -vertex.x : vertex.x;
		vertex.y = bFlipV ? -vertex.y : vertex.y;
		
		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::SPRITE)
		{
			int32_t textureUnit = -1;
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == texture)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}

			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == nullptr)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());
				prevCommand.texture[textureUnit] = texture;

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	uint32_t textureUnit = 0;

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::SPRITE;
	command.texture[textureUnit] = texture;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].uv = uvs[index];
		vertices_[command.startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
		vertices_[command.startVertexIndex + index].unit = textureUnit;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawSprite(ITexture* texture, const GameMath::Vec2f& center, float w, float h, const GameMath::Vec3f& blend, float factor, float rotate, bool bFlipH, bool bFlipV)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, +h2),
	};

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> uvs =
	{
		GameMath::Vec2f(0.0f, 1.0f),
		GameMath::Vec2f(1.0f, 0.0f),
		GameMath::Vec2f(0.0f, 0.0f),
		GameMath::Vec2f(0.0f, 1.0f),
		GameMath::Vec2f(1.0f, 1.0f),
		GameMath::Vec2f(1.0f, 0.0f),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex.x = bFlipH ? -vertex.x : vertex.x;
		vertex.y = bFlipV ? -vertex.y : vertex.y;

		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}

	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::SPRITE)
		{
			int32_t textureUnit = -1;
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == texture)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}

			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == nullptr)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());
				prevCommand.texture[textureUnit] = texture;

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	uint32_t textureUnit = 0;

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::SPRITE;
	command.texture[textureUnit] = texture;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].uv = uvs[index];
		vertices_[command.startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
		vertices_[command.startVertexIndex + index].unit = textureUnit;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawSprite(Atlas2D* atlas, const std::string& name, const GameMath::Vec2f& center, float w, float h, float rotate, bool bFlipH, bool bFlipV)
{
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, +h2),
	};

	const Atlas2D::Block& block = atlas->GetByName(name);
	float atlasWidth = static_cast<float>(atlas->GetWidth());
	float atlasHeight = static_cast<float>(atlas->GetHeight());

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> uvs =
	{
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex.x = bFlipH ? -vertex.x : vertex.x;
		vertex.y = bFlipV ? -vertex.y : vertex.y;

		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}
	
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::SPRITE)
		{
			int32_t textureUnit = -1;
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == atlas)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}

			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == nullptr)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());
				prevCommand.texture[textureUnit] = atlas;

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	uint32_t textureUnit = 0;

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::SPRITE;
	command.texture[textureUnit] = atlas;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].uv = uvs[index];
		vertices_[command.startVertexIndex + index].color = GameMath::Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
		vertices_[command.startVertexIndex + index].unit = textureUnit;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawSprite(Atlas2D* atlas, const std::string& name, const GameMath::Vec2f& center, float w, float h, const GameMath::Vec3f& blend, float factor, float rotate, bool bFlipH, bool bFlipV)
{	
	static const uint32_t MAX_VERTEX_SIZE = 6;
	if (IsFullCommandQueue(MAX_VERTEX_SIZE))
	{
		Flush();
	}

	float w2 = w * 0.5f;
	float h2 = h * 0.5f;

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> vertices =
	{
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, +h2),
		GameMath::Vec2f(-w2, +h2),
		GameMath::Vec2f(-w2, -h2),
		GameMath::Vec2f(+w2, -h2),
		GameMath::Vec2f(+w2, +h2),
	};

	const Atlas2D::Block& block = atlas->GetByName(name);
	float atlasWidth = static_cast<float>(atlas->GetWidth());
	float atlasHeight = static_cast<float>(atlas->GetHeight());

	std::array<GameMath::Vec2f, MAX_VERTEX_SIZE> uvs =
	{
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x               ) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y + block.size.y) / atlasHeight),
		GameMath::Vec2f(static_cast<float>(block.pos.x + block.size.x) / atlasWidth, static_cast<float>(block.pos.y               ) / atlasHeight),
	};

	GameMath::Mat2x2 rotateMat = GameMath::Mat2x2(
		+GameMath::Cos(rotate), -GameMath::Sin(rotate),
		+GameMath::Sin(rotate), +GameMath::Cos(rotate)
	);
	for (auto& vertex : vertices)
	{
		vertex.x = bFlipH ? -vertex.x : vertex.x;
		vertex.y = bFlipV ? -vertex.y : vertex.y;

		vertex = rotateMat * vertex;
		vertex += (center + GameMath::Vec2f(0.375f, 0.375f));
	}
	
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::SPRITE)
		{
			int32_t textureUnit = -1;
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == atlas)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}

			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.texture[unit] == nullptr)
				{
					textureUnit = unit;
					break;
				}
			}

			if (textureUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += static_cast<uint32_t>(vertices.size());
				prevCommand.texture[textureUnit] = atlas;

				for (uint32_t index = 0; index < vertices.size(); ++index)
				{
					vertices_[startVertexIndex + index].position = vertices[index];
					vertices_[startVertexIndex + index].uv = uvs[index];
					vertices_[startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
					vertices_[startVertexIndex + index].unit = textureUnit;
				}

				return;
			}
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	uint32_t textureUnit = 0;

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = static_cast<uint32_t>(vertices.size());
	command.type = RenderCommand::Type::SPRITE;
	command.texture[textureUnit] = atlas;

	for (uint32_t index = 0; index < command.vertexCount; ++index)
	{
		vertices_[command.startVertexIndex + index].position = vertices[index];
		vertices_[command.startVertexIndex + index].uv = uvs[index];
		vertices_[command.startVertexIndex + index].color = GameMath::Vec4f(blend.x, blend.y, blend.z, factor);
		vertices_[command.startVertexIndex + index].unit = textureUnit;
	}

	commandQueue_.push(command);
}

void RenderManager2D::DrawString(TTFont* font, const std::wstring& text, const GameMath::Vec2f& pos, const GameMath::Vec4f& color)
{	
	/** 문자 하나당 정점 6개. */
	uint32_t vertexCount = 6 * static_cast<uint32_t>(text.size());
	if (IsFullCommandQueue(vertexCount))
	{
		Flush();
	}

	float w = 0.0f;
	float h = 0.0f;
	font->MeasureText(text, w, h);

	float atlasWidth = static_cast<float>(font->GetAtlasWidth());
	float atlasHeight = static_cast<float>(font->GetAtlasHeight());
	GameMath::Vec2f currPos = GameMath::Vec2f(pos.x, pos.y - h);

	auto composeVertexData = [&](uint32_t vertexIndex, uint32_t unit)
		{
			for (const auto& unicode : text)
			{
				const Glyph& glyph = font->GetGlyph(static_cast<int32_t>(unicode));

				float uw = static_cast<float>(glyph.pos1.x - glyph.pos0.x);
				float uh = static_cast<float>(glyph.pos1.y - glyph.pos0.y);

				vertices_[vertexIndex + 0].position = GameMath::Vec2f(currPos.x, currPos.y - glyph.yoff);
				vertices_[vertexIndex + 0].uv = GameMath::Vec2f(static_cast<float>(glyph.pos0.x) / atlasWidth, static_cast<float>(glyph.pos0.y) / atlasHeight);
				vertices_[vertexIndex + 0].color = color;
				vertices_[vertexIndex + 0].unit = unit;

				vertices_[vertexIndex + 1].position = GameMath::Vec2f(currPos.x, currPos.y - uh - glyph.yoff);
				vertices_[vertexIndex + 1].uv = GameMath::Vec2f(static_cast<float>(glyph.pos0.x) / atlasWidth, static_cast<float>(glyph.pos1.y) / atlasHeight);
				vertices_[vertexIndex + 1].color = color;
				vertices_[vertexIndex + 1].unit = unit;

				vertices_[vertexIndex + 2].position = GameMath::Vec2f(currPos.x + uw, currPos.y - glyph.yoff);
				vertices_[vertexIndex + 2].uv = GameMath::Vec2f(static_cast<float>(glyph.pos1.x) / atlasWidth, static_cast<float>(glyph.pos0.y) / atlasHeight);
				vertices_[vertexIndex + 2].color = color;
				vertices_[vertexIndex + 2].unit = unit;

				vertices_[vertexIndex + 3].position = GameMath::Vec2f(currPos.x + uw, currPos.y - glyph.yoff);
				vertices_[vertexIndex + 3].uv = GameMath::Vec2f(static_cast<float>(glyph.pos1.x) / atlasWidth, static_cast<float>(glyph.pos0.y) / atlasHeight);
				vertices_[vertexIndex + 3].color = color;
				vertices_[vertexIndex + 3].unit = unit;

				vertices_[vertexIndex + 4].position = GameMath::Vec2f(currPos.x, currPos.y - uh - glyph.yoff);
				vertices_[vertexIndex + 4].uv = GameMath::Vec2f(static_cast<float>(glyph.pos0.x) / atlasWidth, static_cast<float>(glyph.pos1.y) / atlasHeight);
				vertices_[vertexIndex + 4].color = color;
				vertices_[vertexIndex + 4].unit = unit;

				vertices_[vertexIndex + 5].position = GameMath::Vec2f(currPos.x + uw, currPos.y - uh - glyph.yoff);
				vertices_[vertexIndex + 5].uv = GameMath::Vec2f(static_cast<float>(glyph.pos1.x) / atlasWidth, static_cast<float>(glyph.pos1.y) / atlasHeight);
				vertices_[vertexIndex + 5].color = color;
				vertices_[vertexIndex + 5].unit = unit;

				currPos.x += glyph.xadvance;
				vertexIndex += 6;
			}
		};


	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();

		if (prevCommand.drawMode == DrawMode::TRIANGLES && prevCommand.type == RenderCommand::Type::STRING)
		{
			int32_t atlasUnit = -1;
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.font[unit] == font)
				{
					atlasUnit = unit;
					break;
				}
			}

			if (atlasUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += vertexCount;

				composeVertexData(startVertexIndex, atlasUnit);
				return;
			}

			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (prevCommand.font[unit] == nullptr)
				{
					atlasUnit = unit;
					break;
				}
			}

			if (atlasUnit != -1)
			{
				uint32_t startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
				prevCommand.vertexCount += vertexCount;
				prevCommand.font[atlasUnit] = font;

				composeVertexData(startVertexIndex, atlasUnit);
				return;
			}
		}
	}

	uint32_t startVertexIndex = 0;
	if (!commandQueue_.empty())
	{
		RenderCommand& prevCommand = commandQueue_.back();
		startVertexIndex = prevCommand.startVertexIndex + prevCommand.vertexCount;
	}

	uint32_t atlasUnit = 0;

	RenderCommand command;
	command.drawMode = DrawMode::TRIANGLES;
	command.startVertexIndex = startVertexIndex;
	command.vertexCount = vertexCount;
	command.type = RenderCommand::Type::STRING;
	command.font[atlasUnit] = font;

	composeVertexData(command.startVertexIndex, atlasUnit);

	commandQueue_.push(command);
}

void RenderManager2D::Flush()
{
	if (commandQueue_.empty()) /** Command Queue가 비어있으면 동작X */
	{
		return;
	}

	const void* vertexPtr = reinterpret_cast<const void*>(vertices_.data());
	uint32_t bufferByteSize = static_cast<uint32_t>(Vertex::GetStride() * vertices_.size());
	vertexBuffer_->SetBufferData(vertexPtr, bufferByteSize);

	GL_CHECK(glBindVertexArray(vertexArrayObject_));

	while (!commandQueue_.empty())
	{
		RenderCommand command = commandQueue_.front();
		commandQueue_.pop();

		switch (command.type)
		{
		case RenderCommand::Type::SPRITE:
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (command.texture[unit])
				{
					command.texture[unit]->Active(unit);
				}
			}
			break;

		case RenderCommand::Type::STRING:
			for (uint32_t unit = 0; unit < RenderCommand::MAX_TEXTURE_UNIT; ++unit)
			{
				if (command.font[unit])
				{
					command.font[unit]->Active(unit);
				}
			}
			break;
		}

		Shader* shader = shaders_.at(command.type);
		shader->Bind();
		{
			GL_CHECK(glDrawArrays(static_cast<GLenum>(command.drawMode), command.startVertexIndex, command.vertexCount));
		}
		shader->Unbind();
	}

	GL_CHECK(glBindVertexArray(0));
}

bool RenderManager2D::IsFullCommandQueue(uint32_t vertexCount)
{
	if (commandQueue_.empty())
	{
		return false;
	}

	const RenderCommand& command = commandQueue_.back();
	uint32_t index = command.startVertexIndex + command.vertexCount + vertexCount;

	return index >= MAX_VERTEX_BUFFER_SIZE;
}

#pragma warning(pop)