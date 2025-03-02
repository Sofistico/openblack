/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>

#include <entt/locator/locator.hpp>

namespace openblack
{
class RandomNumberManagerInterface;
class LandIslandInterface;
class TempleInteriorInterface;

namespace audio
{
class AudioManagerInterface;
}

namespace filesystem
{
class FileSystemInterface;
}

namespace resources
{
class ResourcesInterface;
}

namespace ecs
{
class Registry;
class MapInterface;
} // namespace ecs

namespace ecs::systems
{
class RenderingSystemInterface;
class DynamicsSystemInterface;
class CameraBookmarkSystemInterface;
class LivingActionSystemInterface;
class TownSystemInterface;
class PathfindingSystemInterface;
class PlayerSystemInterface;

void InitializeGame();
void InitializeLevel(const std::filesystem::path& path);
} // namespace ecs::systems

struct Locator
{
	using filesystem = entt::locator<filesystem::FileSystemInterface>;
	using resources = entt::locator<resources::ResourcesInterface>;
	using rng = entt::locator<RandomNumberManagerInterface>;
	using terrainSystem = entt::locator<LandIslandInterface>;
	using audio = entt::locator<audio::AudioManagerInterface>;
	using rendereringSystem = entt::locator<ecs::systems::RenderingSystemInterface>;
	using dynamicsSystem = entt::locator<ecs::systems::DynamicsSystemInterface>;
	using cameraBookmarkSystem = entt::locator<ecs::systems::CameraBookmarkSystemInterface>;
	using livingActionSystem = entt::locator<ecs::systems::LivingActionSystemInterface>;
	using townSystem = entt::locator<ecs::systems::TownSystemInterface>;
	using pathfindingSystem = entt::locator<ecs::systems::PathfindingSystemInterface>;
	using entitiesRegistry = entt::locator<ecs::Registry>;
	using entitiesMap = entt::locator<ecs::MapInterface>;
	using playerSystem = entt::locator<ecs::systems::PlayerSystemInterface>;
	using temple = entt::locator<TempleInteriorInterface>;
};
} // namespace openblack
