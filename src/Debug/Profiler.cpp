/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Profiler.h"

#include <cinttypes>

#include <bgfx/bgfx.h>
#include <imgui_widget_flamegraph.h>

#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"

#include "../Profiler.h"

using namespace openblack::debug::gui;

Profiler::Profiler()
    : Window("Profiler", ImVec2(650.0f, 800.0f))
{
}

void Profiler::Open()
{
	Window::Open();
	Game::Instance()->GetConfig().bgfxProfile = true;
}

void Profiler::Close()
{
	Window::Close();
	Game::Instance()->GetConfig().bgfxProfile = false;
}

void Profiler::Draw(Game& game)
{
	auto& config = game.GetConfig();

	using namespace ecs::components;

	const bgfx::Stats* stats = bgfx::getStats();
	const double toMsCpu = 1000.0 / stats->cpuTimerFreq;
	const double toMsGpu = 1000.0 / stats->gpuTimerFreq;
	const double frameMs = double(stats->cpuTimeFrame) * toMsCpu;
	_times.PushBack(static_cast<float>(frameMs));
	_fps.PushBack(static_cast<float>(1000.0 / frameMs));

	std::array<char, 256> frameTextOverlay;
	std::snprintf(frameTextOverlay.data(), frameTextOverlay.size(), "%.3fms, %.1f FPS", _times.Back(), _fps.Back());

	ImGui::Text("Submit CPU %0.3f, GPU %0.3f (Max GPU Latency: %d)", double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu,
	            double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu, stats->maxGpuLatency);
	ImGui::Text("Wait Submit %0.3f, Wait Render %0.3f", stats->waitSubmit * toMsCpu, stats->waitRender * toMsCpu);

	ImGui::Columns(5);
	ImGui::Checkbox("Sky", &config.drawSky);
	ImGui::NextColumn();
	ImGui::Checkbox("Water", &config.drawWater);
	ImGui::NextColumn();
	ImGui::Checkbox("Island", &config.drawIsland);
	ImGui::NextColumn();
	ImGui::Checkbox("Entities", &config.drawEntities);
	ImGui::NextColumn();
	ImGui::Checkbox("Sprites", &config.drawSprites);
	ImGui::NextColumn();
	ImGui::Checkbox("TestModel", &config.drawTestModel);
	ImGui::NextColumn();
	ImGui::Checkbox("Debug Cross", &config.drawDebugCross);
	ImGui::Columns(1);

	auto width = ImGui::GetColumnWidth() - ImGui::CalcTextSize("Frame").x;
	ImGui::PlotHistogram("Frame", _times.values.data(), decltype(_times)::k_BufferSize, _times.offset, frameTextOverlay.data(),
	                     0.0f, FLT_MAX, ImVec2(width, 45.0f));

	ImGui::Text("Primitives Triangles %u, Triangle Strips %u, Lines %u "
	            "Line Strips %u, Points %u",
	            stats->numPrims[0], stats->numPrims[1], stats->numPrims[2], stats->numPrims[3], stats->numPrims[4]);
	ImGui::Columns(2);
	ImGui::Text("Num Entities %u, Trees %u", static_cast<uint32_t>(Locator::entitiesRegistry::value().Size<Transform>()),
	            static_cast<uint32_t>(Locator::entitiesRegistry::value().Size<Tree>()));
	ImGui::Text("Num Draw %u, Num Compute %u, Num Blit %u", stats->numDraw, stats->numCompute, stats->numBlit);
	ImGui::Text("Num Buffers Index %u, Vertex %u", stats->numIndexBuffers, stats->numVertexBuffers);
	ImGui::Text("Num Dynamic Buffers Index %u, Vertex %u", stats->numDynamicIndexBuffers, stats->numDynamicVertexBuffers);
	ImGui::Text("Num Transient Buffers Index %u, Vertex %u", stats->transientIbUsed, stats->transientVbUsed);
	ImGui::NextColumn();
	ImGui::Text("Num Vertex Layouts %u", stats->numVertexLayouts);
	ImGui::Text("Num Textures %u, FrameBuffers %u", stats->numTextures, stats->numFrameBuffers);
	ImGui::Text("Memory Texture %" PRId64 ", RenderTarget %" PRId64, stats->textureMemoryUsed, stats->rtMemoryUsed);
	ImGui::Text("Num Programs %u, Num Shaders %u, Uniforms %u", stats->numPrograms, stats->numShaders, stats->numUniforms);
	ImGui::Text("Num Occlusion Queries %u", stats->numOcclusionQueries);

	ImGui::Columns(1);

	auto& entry = game.GetProfiler().GetEntries().at(game.GetProfiler().GetEntryIndex(-1));

	ImGuiWidgetFlameGraph::PlotFlame(
	    "CPU",
	    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
		    const auto* entry = reinterpret_cast<const openblack::Profiler::Entry*>(data);
		    const auto& stage = entry->stages.at(idx);
		    if (startTimestamp != nullptr)
		    {
			    std::chrono::duration<float, std::milli> fltStart = stage.start - entry->frameStart;
			    *startTimestamp = fltStart.count();
		    }
		    if (endTimestamp != nullptr)
		    {
			    *endTimestamp = stage.end.time_since_epoch().count() / 1e6f;

			    std::chrono::duration<float, std::milli> fltEnd = stage.end - entry->frameStart;
			    *endTimestamp = fltEnd.count();
		    }
		    if (level != nullptr)
		    {
			    *level = stage.level;
		    }
		    if (caption != nullptr)
		    {
			    *caption = openblack::Profiler::k_StageNames.at(idx).data();
		    }
	    },
	    &entry, static_cast<uint8_t>(openblack::Profiler::Stage::_count), 0, "Main Thread", 0, FLT_MAX, ImVec2(width, 0));

	ImGuiWidgetFlameGraph::PlotFlame(
	    "GPU",
	    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
		    const auto* stats = reinterpret_cast<const bgfx::Stats*>(data);
		    if (startTimestamp != nullptr)
		    {
			    *startTimestamp = static_cast<float>(1000.0 * (stats->viewStats[idx].gpuTimeBegin - stats->gpuTimeBegin) /
			                                         static_cast<double>(stats->gpuTimerFreq));
		    }
		    if (endTimestamp != nullptr)
		    {
			    *endTimestamp = static_cast<float>(1000.0 * (stats->viewStats[idx].gpuTimeEnd - stats->gpuTimeBegin) /
			                                       static_cast<double>(stats->gpuTimerFreq));
		    }
		    if (level != nullptr)
		    {
			    *level = 0;
		    }
		    if (caption != nullptr)
		    {
			    *caption = stats->viewStats[idx].name;
		    }
	    },
	    stats, stats->numViews, 0, "GPU Frame", 0,
	    static_cast<float>(1000.0 * (stats->gpuTimeEnd - stats->gpuTimeBegin) / static_cast<double>(stats->gpuTimerFreq)),
	    ImVec2(width, 0));

	ImGui::Columns(2);
	if (ImGui::CollapsingHeader("Details (CPU)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		std::chrono::duration<float, std::milli> frameDuration = entry.frameEnd - entry.frameStart;
		ImGui::Text("Full Frame: %0.3f", frameDuration.count());
		auto cursorX = ImGui::GetCursorPosX();
		auto indentSize = ImGui::CalcTextSize("    ").x;

		for (uint8_t i = 0; const auto& stage : entry.stages)
		{
			std::chrono::duration<float, std::milli> duration = stage.end - stage.start;
			ImGui::SetCursorPosX(cursorX + indentSize * stage.level);
			ImGui::Text("    %s: %0.3f", openblack::Profiler::k_StageNames.at(i).data(), duration.count());
			if (stage.level == 0)
			{
				frameDuration -= duration;
			}
			++i;
		}
		ImGui::Text("    Unaccounted: %0.3f", frameDuration.count());
	}
	ImGui::NextColumn();
	if (ImGui::CollapsingHeader("Details (GPU)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto frameDuration = stats->gpuTimeEnd - stats->gpuTimeBegin;
		ImGui::Text("Full Frame: %0.3f", 1000.0f * frameDuration / static_cast<double>(stats->gpuTimerFreq));

		for (uint8_t i = 0; i < stats->numViews; ++i)
		{
			auto const& viewStat = stats->viewStats[i];
			int64_t gpuTimeElapsed = viewStat.gpuTimeEnd - viewStat.gpuTimeBegin;

			ImGui::Text("    %s: %0.3f", viewStat.name, 1000.0f * gpuTimeElapsed / static_cast<double>(stats->gpuTimerFreq));
			frameDuration -= gpuTimeElapsed;
		}
		ImGui::Text("    Unaccounted: %0.3f", 1000.0f * frameDuration / static_cast<double>(stats->gpuTimerFreq));
	}
	ImGui::Columns(1);
}

void Profiler::Update([[maybe_unused]] Game& game, [[maybe_unused]] const Renderer& renderer) {}

void Profiler::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void Profiler::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
