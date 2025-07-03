#include "Game/Player.hpp"
#include "Game/GameCommon.h"
#include "Game/PlayerDefinition.hpp"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/DebugRender.hpp"

Player::Player(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color)
	:Entity(owner, position, orientation, color),
	 m_playerDef(PlayerDefinition::GetPlayerByName("Runner"))
{
	m_position = position;
	m_orientation = orientation;
	m_gravityForce = GRAVITY_FORCE;
	InitializePlayerData();
	InitializePlayerGeometry();
}

void Player::InitializePlayerData()
{
	m_physicsHeight = m_playerDef->m_physicsHeight;
	m_physicsRadius = m_playerDef->m_physicsRadius;
	m_playerJumpForce = m_playerDef->m_jumpForce;
}

void Player::InitializePlayerGeometry()
{
	Rgba8 bodyColor = m_color;
	float bodyRadius = m_physicsRadius * 0.6f;
	float bodyHeight = m_physicsHeight * 0.6f;

	// Body
	Vec3 bodyCenter = m_position + Vec3(0.f, 0.f, bodyHeight * 0.25f);
	AddVertsForSphere3D(m_playerVerts, bodyCenter, bodyRadius, bodyColor);

	// Arms
	float armRadius = bodyRadius * 0.3f;
	Vec3 leftShoulder = bodyCenter + Vec3(0.f, bodyRadius - armRadius * 0.5f, 0.f);
	Vec3 leftHand = leftShoulder + Vec3(0.f, bodyRadius * 0.8f, -bodyHeight * 0.3f);
	Vec3 rightShoulder = bodyCenter + Vec3(0.f, -bodyRadius + armRadius * 0.5f, 0.f);
	Vec3 rightHand = rightShoulder + Vec3(0.f, -bodyRadius * 0.8f, -bodyHeight * 0.3f);
	AddVertsForCylinder3D(m_playerVerts, leftShoulder, leftHand, bodyRadius * 0.3f, bodyColor);
	AddVertsForCylinder3D(m_playerVerts, rightShoulder, rightHand, bodyRadius * 0.3f, bodyColor);

	// Legs
	Vec3 leftHip = m_position + Vec3(0.f, bodyRadius * 0.5f, 0.15f);
	Vec3 leftFoot = leftHip + Vec3(0.f, bodyRadius * 0.25f, -bodyHeight * 0.9f);
	Vec3 rightHip = m_position + Vec3(0.f, -bodyRadius * 0.5f, 0.15f);
	Vec3 rightFoot = rightHip + Vec3(0.f, -bodyRadius * 0.25f, -bodyHeight * 0.9f);
	AddVertsForCylinder3D(m_playerVerts, leftHip, leftFoot, bodyRadius * 0.35f, bodyColor);
	AddVertsForCylinder3D(m_playerVerts, rightHip, rightFoot, bodyRadius * 0.35f, bodyColor);

	// Antennas
	float antennaLength = bodyHeight * 0.5f;
	float antennaRadius = bodyRadius * 0.1f;
	float antennaBallRadius = antennaRadius * 1.9f;
	float antennaBaseOffsetZ = -bodyRadius * 0.2f;

	Vec3 topOfHead = bodyCenter + Vec3(0.f, 0.f, bodyRadius + antennaBaseOffsetZ);
	Vec3 leftAntennaDir = Vec3(0.4f, 0.4f, 1.0f).GetNormalized();
	Vec3 rightAntennaDir = Vec3(-0.4f, -0.4f, 1.0f).GetNormalized();

	// Left antenna
	Vec3 antennaBaseLeft = topOfHead + Vec3(0.f, bodyRadius * 0.3f, 0.f);
	Vec3 antennaTipLeft = antennaBaseLeft + leftAntennaDir * antennaLength;
	AddVertsForCylinder3D(m_playerVerts, antennaBaseLeft, antennaTipLeft, antennaRadius, bodyColor);
	AddVertsForSphere3D(m_playerVerts, antennaTipLeft, antennaBallRadius, bodyColor);

	// Right antenna
	Vec3 antennaBaseRight = topOfHead + Vec3(0.f, -bodyRadius * 0.3f, 0.f);
	Vec3 antennaTipRight = antennaBaseRight + rightAntennaDir * antennaLength;
	AddVertsForCylinder3D(m_playerVerts, antennaBaseRight, antennaTipRight, antennaRadius, bodyColor);
	AddVertsForSphere3D(m_playerVerts, antennaTipRight, antennaBallRadius, bodyColor);
}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	PlayerInput(deltaSeconds);

	m_velocity.z += m_gravityForce * deltaSeconds;

	// Here I am clamping gravity so we don't fall super fast
	m_velocity.z = GetClamped(m_velocity.z, MAX_FALL_SPEED, m_playerJumpForce);

	m_position += m_velocity * deltaSeconds;

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_drawDebug = !m_drawDebug;
	}
}

void Player::DrawDebug() const
{
	// Draw debug physics cylinder
	if (m_drawDebug)
	{
		Vec3 base = m_position - Vec3(0.f, 0.f, m_physicsHeight * 0.5f);
		Vec3 top = m_position + Vec3(0.f, 0.f, m_physicsHeight * 0.5f);
		DebugAddWorldWireCylinder(base, top, m_physicsRadius, 0.f, Rgba8::RED, Rgba8::RED);
	}
}

void Player::Render() const
{
	DrawDebug();

	g_theRenderer->SetModelConstants(GetModelToWorldTransform());
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_playerVerts);
}

Vec3 Player::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees, 2.f);
}

Camera Player::GetPlayerCamera() const
{
	return m_playerCamera;
}

void Player::PlayerInput(float deltaseconds)
{
	UNUSED(deltaseconds);

	Vec3 forward = GetModelToWorldTransform().GetIBasis3D();
	Vec3 left = GetModelToWorldTransform().GetJBasis3D();
	float playerMoveSpeed = m_playerDef->m_moveSpeed;
	float playerStrafeSpeed = m_playerDef->m_strafeSpeed;

	// Constant forward speed
	Vec3 horizontalVelocity = forward * playerMoveSpeed;

	// Left and right movement
	if (g_theInput->IsKeyDown('A')) 
	{
		horizontalVelocity += left * playerStrafeSpeed;
	}
	if (g_theInput->IsKeyDown('D')) 
	{
		horizontalVelocity -= left * playerStrafeSpeed;
	}

	m_velocity = Vec3(horizontalVelocity.x, horizontalVelocity.y, m_velocity.z);

	// Jumping movement
	if (g_theInput->WasKeyJustPressed(' ') && m_isGrounded)
	{
		m_velocity.z = m_playerJumpForce;
		m_isGrounded = false;
	}

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
}

void Player::Respawn()
{
	m_position = Vec3::ZERO;
	m_velocity = Vec3::ZERO;
	m_orientation = EulerAngles::ZERO;
	m_isGrounded = false;
}
