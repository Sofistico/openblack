/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "Graphics/RenderPass.h"

namespace openblack
{
struct BgfxCallback;
class Camera;
class GameWindow;
class Game;
class L3DAnim;
class L3DMesh;
class L3DSubMesh;
class Profiler;
class Sky;
class Water;

namespace ecs
{
class Registry;
}

namespace graphics
{
class FrameBuffer;
class Mesh;
class ShaderManager;
class ShaderProgram;
} // namespace graphics

class Renderer
{

public:
	struct DrawSceneDesc
	{
		Profiler& profiler;
		const Camera* camera;
		const graphics::FrameBuffer* frameBuffer;
		const Sky& sky;
		const Water& water;
		const ecs::Registry& entities;
		uint32_t time;
		float timeOfDay;
		float bumpMapStrength;
		float smallBumpMapStrength;
		graphics::RenderPass viewId;
		bool drawSky;
		bool drawWater;
		bool drawIsland;
		bool drawEntities;
		bool drawSprites;
		bool drawTestModel;
		bool drawDebugCross;
		bool drawBoundingBoxes;
		bool cullBack;
		bool bgfxDebug;
		bool bgfxProfile;
		bool wireframe;
	};

	struct L3DMeshSubmitDesc
	{
		graphics::RenderPass viewId;
		const graphics::ShaderProgram* program;
		uint64_t state;
		uint32_t rgba;
		const glm::mat4* modelMatrices;
		uint8_t matrixCount;
		const bgfx::DynamicVertexBufferHandle* instanceBuffer;
		uint32_t instanceStart;
		uint32_t instanceCount;
		bool isSky;
		float skyType;
		bool drawAll; ///< For use in the mesh viewer
		bool morphWithTerrain;
	};

	Renderer() = delete;
	explicit Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync);

	virtual ~Renderer();

	[[nodiscard]] graphics::ShaderManager& GetShaderManager() const;

	void UpdateDebugCrossUniforms(const glm::mat4& pose);

	void ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height, uint32_t clearColor = 0x274659ff) const;

	void DrawScene(const DrawSceneDesc& drawDesc) const;
	void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const;
	void Frame();
	void RequestScreenshot(const std::filesystem::path& filepath);

	void Reset(uint16_t width, uint16_t height) const;

private:
	void DrawFootprintPass(const DrawSceneDesc& drawDesc) const;
	void DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc, bool preserveState) const;
	void DrawPass(const DrawSceneDesc& desc) const;

	std::unique_ptr<graphics::ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;
	uint32_t _bgfxReset;

	std::unique_ptr<graphics::Mesh> _debugCross;
	std::unique_ptr<graphics::Mesh> _plane;
	glm::mat4 _debugCrossPose;
};
} // namespace openblack
