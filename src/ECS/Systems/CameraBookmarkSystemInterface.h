/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <chrono>
#include <memory>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs::systems
{

class CameraBookmarkSystemInterface
{
public:
	virtual bool Initialize() = 0;
	virtual void Update(const std::chrono::microseconds& dt) const = 0;
	[[nodiscard]] virtual const std::array<entt::entity, 8>& GetBookmarks() const = 0;
	virtual void SetBookmark(uint8_t index, const glm::vec3& position) const = 0;
	virtual void ClearBookmark(uint8_t index) const = 0;
};

} // namespace openblack::ecs::systems
