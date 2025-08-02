#pragma once
#include "Engine/Renderer/Camera.h"
#include "Engine/Core/Vertex_PCU.h"
#include <vector>
#include <string>
// -----------------------------------------------------------------------------
class  Game;
class  AnimationGroup;
struct PlayerDefinition;
class  Clock;
// -----------------------------------------------------------------------------
class Player
{
public:
	Player(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color, PlayerDefinition* def);
	~Player();
	void InitializePlayerData();
	void InitializePlayerGeometry();

	void Update(float deltaSeconds);
	void UpdateAnimation();
	void ToggleShadow();

	void  DrawDebug() const;
	void  Render() const;
	Mat44 GetModelToWorldTransform() const;
	Mat44 GetShadowToWorldTransform() const;

	Vec3  GetForwardNormal() const;
	void  PlayerInput(float deltaseconds);
	void  Respawn();
	void  PlayAnimation(std::string const& name);

	Vec3 m_gravityDirection = Vec3(0.f, 0.f, -1.f);

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
	PlayerDefinition* m_playerDef = nullptr;

private:
	Camera m_playerCamera;
	std::vector<Vertex_PCU> m_playerVerts;
	std::vector<Vertex_PCU> m_overlayVerts;
	bool m_drawDebug = false;
	float m_playerJumpForce = 0.0f;

	Clock* m_animationClock = nullptr;
	AnimationGroup* m_animGroup = nullptr;
	bool m_isTurning = false;
	bool m_showShadow = true;
};