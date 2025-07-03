#pragma once
#include "Engine/Math/Vec3.h"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.h"
// -----------------------------------------------------------------------------
class Game;
class Mat44;
// -----------------------------------------------------------------------------
class Entity
{
public:
	Entity(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color);
	virtual ~Entity();

	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual Mat44 GetModelToWorldTransform() const;

public:
	Game* m_game = nullptr;
	Rgba8 m_color = Rgba8::WHITE;
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;
	float  m_physicsRadius = 0.0f;
	float  m_physicsHeight = 0.0f;
	EulerAngles m_orientation = EulerAngles::ZERO;
	float  m_gravityForce = -30.f;
	bool   m_isGrounded = false;
};