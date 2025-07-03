#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Camera.h"
#include "Engine/Core/Vertex_PCU.h"
#include <vector>
// -----------------------------------------------------------------------------
struct PlayerDefinition;
// -----------------------------------------------------------------------------
class Player : public Entity
{
public:
	Player(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color);

	void InitializePlayerData();
	void InitializePlayerGeometry();

	~Player();

	void Update(float deltaSeconds) override;

	void DrawDebug() const;
	void Render() const override;

	Vec3 GetForwardNormal() const;
	Camera GetPlayerCamera() const;
	void PlayerInput(float deltaseconds);
	void Respawn();

	Vec3 m_gravityDirection = Vec3(0.f, 0.f, -1.f);

private:
	PlayerDefinition* m_playerDef = nullptr;
	Camera m_playerCamera;
	std::vector<Vertex_PCU> m_playerVerts;
	std::vector<Vertex_PCU> m_overlayVerts;
	bool m_drawDebug = false;
	float m_playerJumpForce = 0.0f;
};