/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ECS/Components/Transform.h"

union SDL_Event;

namespace openblack
{
class CameraInterface
{
public:
	[[nodiscard]] virtual glm::mat4 GetViewMatrix() const = 0;
	[[nodiscard]] virtual const glm::mat4& GetProjectionMatrix() const = 0;
	[[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix() const = 0;

	[[nodiscard]] virtual std::optional<ecs::components::Transform> RaycastMouseToLand() = 0;
	virtual void FlyInit() = 0;
	virtual void StartFlight() = 0;
	virtual void ResetVelocities() = 0;

	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
	/// Get rotation as euler angles in radians
	[[nodiscard]] virtual glm::vec3 GetRotation() const = 0;
	[[nodiscard]] virtual glm::vec3 GetVelocity() const = 0;
	[[nodiscard]] virtual float GetMaxSpeed() const = 0;

	virtual void SetPosition(const glm::vec3& position) = 0;

	/// Set rotation as euler angles in radians
	virtual void SetRotation(const glm::vec3& eulerRadians) = 0;

	virtual void SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip) = 0;
	virtual void SetProjectionMatrix(const glm::mat4x4& projection) = 0;

	[[nodiscard]] virtual glm::vec3 GetForward() const = 0;
	[[nodiscard]] virtual glm::vec3 GetRight() const = 0;
	[[nodiscard]] virtual glm::vec3 GetUp() const = 0;

	[[nodiscard]] virtual std::unique_ptr<CameraInterface> Reflect(const glm::vec4& relectionPlane) const = 0;

	virtual void DeprojectScreenToWorld(glm::ivec2 screenPosition, glm::ivec2 screenSize, glm::vec3& outWorldOrigin,
	                                    glm::vec3& outWorldDirection) = 0;
	virtual bool ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition) const = 0;

	virtual void Update(std::chrono::microseconds dt) = 0;
	virtual void ProcessSDLEvent(const SDL_Event&) = 0;

	virtual void HandleKeyboardInput(const SDL_Event&) = 0;
	virtual void HandleMouseInput(const SDL_Event&) = 0;

	[[nodiscard]] virtual glm::mat4 GetRotationMatrix() const = 0;
};
} // namespace openblack
