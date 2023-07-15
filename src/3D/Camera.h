/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "CameraInterface.h"

namespace openblack
{

class Camera: public CameraInterface
{
public:
	Camera(glm::vec3, glm::vec3);
	Camera()
	    : Camera(glm::vec3(0.0f), glm::vec3(0.0f))
	{
	}

	virtual ~Camera() = default;

	[[nodiscard]] glm::mat4 GetViewMatrix() const override;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const override { return _projectionMatrix; }
	[[nodiscard]] glm::mat4 GetViewProjectionMatrix() const override;

	[[nodiscard]] std::optional<ecs::components::Transform> RaycastMouseToLand() override;
	void FlyInit() override;
	void StartFlight() override;
	void ResetVelocities() override;

	[[nodiscard]] glm::vec3 GetPosition() const override { return _position; }
	/// Get rotation as euler angles in radians
	[[nodiscard]] glm::vec3 GetRotation() const override { return _rotation; }
	[[nodiscard]] glm::vec3 GetVelocity() const override { return _velocity; }
	[[nodiscard]] float GetMaxSpeed() const override { return _maxMovementSpeed; }

	void SetPosition(const glm::vec3& position) override { _position = position; }
	/// Set rotation as euler angles in radians
	void SetRotation(const glm::vec3& eulerRadians) override { _rotation = eulerRadians; }

	void SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip) override;
	void SetProjectionMatrix(const glm::mat4x4& projection) override { _projectionMatrix = projection; }

	[[nodiscard]] glm::vec3 GetForward() const override;
	[[nodiscard]] glm::vec3 GetRight() const override;
	[[nodiscard]] glm::vec3 GetUp() const override;

	[[nodiscard]] std::unique_ptr<CameraInterface> Reflect(const glm::vec4& relectionPlane) const override;

	void DeprojectScreenToWorld(glm::ivec2 screenPosition, glm::ivec2 screenSize, glm::vec3& outWorldOrigin,
	                            glm::vec3& outWorldDirection) override;
	bool ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition) const override;

	void Update(std::chrono::microseconds dt) override;
	void ProcessSDLEvent(const SDL_Event&) override;

	void HandleKeyboardInput(const SDL_Event&) override;
	void HandleMouseInput(const SDL_Event&) override;

	[[nodiscard]] glm::mat4 GetRotationMatrix() const override;

protected:
	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::vec3 _dv;
	glm::vec3 _dwv;
	glm::vec3 _dsv;
	glm::vec3 _ddv;
	glm::vec3 _duv;
	glm::vec3 _drv;
	glm::mat4 _projectionMatrix;
	glm::vec3 _velocity;
	glm::vec3 _hVelocity;
	glm::vec3 _rotVelocity;
	float _accelFactor;
	float _movementSpeed;
	float _maxMovementSpeed;
	float _maxRotationSpeed;
	bool _lmouseIsDown;
	bool _mmouseIsDown;
	bool _mouseIsMoving;
	glm::ivec2 _mouseFirstClick;
	bool _shiftHeld;
	glm::ivec2 _handScreenVec;
	float _handDragMult;
	bool _flyInProgress;
	float _flyDist;
	float _flySpeed;
	float _flyStartAngle;
	float _flyEndAngle;
	float _flyThreshold;
	float _flyProgress;
	glm::vec3 _flyFromPos;
	glm::vec3 _flyToNorm;
	glm::vec3 _flyFromTan;
	glm::vec3 _flyToPos;
	glm::vec3 _flyToTan;
	glm::vec3 _flyPrevPos;
};

class ReflectionCamera final: public Camera
{
public:
	ReflectionCamera()
	    : ReflectionCamera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec4(0.0f))
	{
	}
	ReflectionCamera(glm::vec3 position, glm::vec3 rotation, glm::vec4 reflectionPlane)
	    : Camera(position, rotation)
	    , _reflectionPlane(reflectionPlane)
	{
	}
	[[nodiscard]] glm::mat4 GetViewMatrix() const override;

private:
	glm::vec4 _reflectionPlane;
	void ReflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const;
};
} // namespace openblack
