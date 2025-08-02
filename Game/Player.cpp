#include "Game/Player.hpp"
#include "Game/GameCommon.h"
#include "Game/Game.h"
#include "Game/Level.hpp"
#include "Game/AnimationGroup.hpp"
#include "Game/PlayerDefinition.hpp"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Renderer/Renderer.h"

Player::Player(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color, PlayerDefinition* def)
	: m_game(owner),
	  m_position(position),
	  m_orientation(orientation),
	  m_color(color),
	  m_playerDef(def),
	  m_animationClock(new Clock(*g_theGame->m_gameClock))
{
	m_position = position;
	m_orientation = orientation;
	InitializePlayerData();
	InitializePlayerGeometry();

	if (m_playerDef->m_isVisible && !m_playerDef->m_animationGroups.empty())
	{
		m_animGroup = m_playerDef->m_animationGroups[0];
	}
}

void Player::InitializePlayerData()
{
	m_gravityForce = GRAVITY_FORCE;
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

	if (m_animGroup == nullptr)
	{
		return;
	}

	UpdateAnimation();

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_drawDebug = !m_drawDebug;
	}
	if (g_theInput->WasKeyJustPressed('L'))
	{
		ToggleShadow();
	}
}

void Player::UpdateAnimation()
{
	float animDuration = m_animGroup->m_anims[0].GetDuration();
	if (m_animationClock->GetTotalSeconds() > animDuration && m_animGroup->m_playbackMode == SpriteAnimPlaybackType::ONCE)
	{
		if (m_animGroup != m_playerDef->m_animationGroups[0])
		{
			m_animGroup = m_playerDef->m_animationGroups[0];
			m_animationClock->Reset();
		}
	}
	if (m_animGroup->m_scaleBySpeed)
	{
		m_animationClock->SetTimeScale(m_velocity.GetLength() / m_playerDef->m_moveSpeed);
	}
	else
	{
		m_animationClock->SetTimeScale(1.f);
	}
}

void Player::ToggleShadow()
{
	m_showShadow = !m_showShadow;
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

	if (!m_playerDef->m_isVisible)
	{
		return;
	}


	// Setting billboard types
	Mat44 localToWorldTransform;
	if (m_playerDef->m_billboardType == BillboardType::NONE)
	{
		localToWorldTransform = GetModelToWorldTransform();
	}
	else
	{
		if (m_playerDef->m_billboardType == BillboardType::WORLD_UP_FACING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::WORLD_UP_FACING, g_theGame->m_gameWorldCamera.GetCameraToWorldTransform(), m_position));
		}
		else if (m_playerDef->m_billboardType == BillboardType::FULL_OPPOSING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::FULL_OPPOSING, g_theGame->m_gameWorldCamera.GetCameraToWorldTransform(), m_position));
		}
		else if (m_playerDef->m_billboardType == BillboardType::WORLD_UP_OPPOSING)
		{
			localToWorldTransform.Append(GetBillboardMatrix(BillboardType::WORLD_UP_OPPOSING, g_theGame->m_gameWorldCamera.GetCameraToWorldTransform(), m_position));
		}
		else
		{
			localToWorldTransform = GetModelToWorldTransform();
		}
	}

	Vec2 playerToActorDirectionXY = (m_position - g_theGame->m_gameWorldCamera.GetPosition()).GetXY();
	Vec3 playerToActorDirection = playerToActorDirectionXY.GetNormalized().GetAsVec3();
	Vec3 viewingDirection = GetModelToWorldTransform().GetOrthonormalInverse().TransformVectorQuantity3D(playerToActorDirection);

	SpriteAnimDefinition anim = m_animGroup->GetAnimDirection(viewingDirection);
	SpriteDefinition spriteDef = anim.GetSpriteDefAtTime(static_cast<float>(m_animationClock->GetTotalSeconds()));
	AABB2 spriteUVs = spriteDef.GetUVs();

	Vec3 spriteOffsetSize = -Vec3(0.f, m_playerDef->m_spriteSize.x, m_playerDef->m_spriteSize.y);
	Vec3 spriteOffsetPivot = Vec3(0.f, m_playerDef->m_spritePivot.x, m_playerDef->m_spritePivot.y);
	Vec3 spriteOffset = (spriteOffsetSize * spriteOffsetPivot);

	Vec3 bL = Vec3::ZERO;
	Vec3 bR = (Vec3::YAXE * m_playerDef->m_spriteSize.x);
	Vec3 tR = (Vec3::YAXE * m_playerDef->m_spriteSize.x) + (Vec3::ZAXE * m_playerDef->m_spriteSize.y);
	Vec3 tL = (Vec3::ZAXE * m_playerDef->m_spriteSize.y);

	std::vector<Vertex_PCU> unlitVerts;
	std::vector<Vertex_PCUTBN> litVertexes;

	// Drawing player sprite
	if (m_playerDef->m_renderRounded)
	{
		litVertexes.reserve(10000);
		AddVertsForRoundedQuad3D(litVertexes, bL, bR, tR, tL, Rgba8::WHITE, spriteUVs);
		TransformVertexArrayTBN3D(litVertexes, Mat44::MakeTranslation3D(spriteOffset));
	}
	else
	{
		litVertexes.reserve(10000);
		AddVertsForQuad3D(litVertexes, bL, bR, tR, tL, Rgba8::WHITE, spriteUVs);
		TransformVertexArrayTBN3D(litVertexes, Mat44::MakeTranslation3D(spriteOffset));
	}
	g_theRenderer->SetModelConstants(localToWorldTransform);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindShader(m_playerDef->m_shader);
	g_theRenderer->BindTexture(&spriteDef.GetTexture());
	g_theRenderer->DrawVertexArray(litVertexes);

	// Drawing planar projected shadow
	if (!m_isGrounded)
	{
		std::vector<Vertex_PCU> shadowVerts;
		Vec3 shadowBL = Vec3(-0.5f, -0.25f, 0.f);
		Vec3 shadowBR = Vec3(0.5f, -0.25f, 0.f);
		Vec3 shadowTR = Vec3(0.5f, 0.25f, 0.f);
		Vec3 shadowTL = Vec3(-0.5f, 0.25f, 0.f);
		Rgba8 shadowColor = Rgba8(0, 0, 0, 180);
		AddVertsForQuad3D(shadowVerts, shadowBL, shadowBR, shadowTR, shadowTL, shadowColor);

		Mat44 shadowTransform = GetShadowToWorldTransform();
		if (shadowTransform.GetTranslation3D() == Vec3::ZERO)
		{
			return;
		}

		if (m_showShadow)
		{
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			g_theRenderer->SetModelConstants(shadowTransform);
			g_theRenderer->BindShader(m_playerDef->m_shader);
			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray(m_playerVerts);
		}
	}
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	EulerAngles orientation;
	modelToWorldMatrix.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}

Mat44 Player::GetShadowToWorldTransform() const
{
	Mat44 shadowToWorldMatrix;
	float maxDist = 100.f;

	if (m_position.z > 0.f || m_velocity.z > 0.f)
	{
		Vec3 intersectionPoint;

		if (g_theGame->m_currentLevel->RaycastDown(m_position, maxDist, intersectionPoint))
		{
			Vec3 planarShadowOffset = Vec3(0.f, 0.f, 0.1f);
			shadowToWorldMatrix.SetTranslation3D(intersectionPoint + planarShadowOffset);
			EulerAngles orientation;
			shadowToWorldMatrix.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());
		}
		else
		{
			shadowToWorldMatrix.SetTranslation3D(Vec3(0.f, 0.f, 0.f));
			return shadowToWorldMatrix;
		}
	}

	return shadowToWorldMatrix;
}

Vec3 Player::GetForwardNormal() const
{
	return Vec3::MakeFromPolarDegrees(m_orientation.m_pitchDegrees, m_orientation.m_yawDegrees, 2.f);
}

void Player::PlayerInput(float deltaseconds)
{
	UNUSED(deltaseconds);

	Vec3 forward = GetModelToWorldTransform().GetIBasis3D();
	Vec3 left = GetModelToWorldTransform().GetJBasis3D();
	float playerMoveSpeed = m_playerDef->m_moveSpeed;
	float playerStrafeSpeed = m_playerDef->m_strafeSpeed;

	// Jumping movement
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) && m_isGrounded)
	{
		PlayAnimation("Jump");
		m_velocity.z = m_playerJumpForce;
		m_isGrounded = false;
	}

	// Constant forward speed
	Vec3 horizontalVelocity = forward * playerMoveSpeed;

	// Left and right movement
	if (!m_isGrounded || g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		if (g_theInput->IsKeyDown('A')) 
		{
			horizontalVelocity += left * playerStrafeSpeed;
		}
		if (g_theInput->IsKeyDown('D')) 
		{
			horizontalVelocity -= left * playerStrafeSpeed;
		}
	}
	else
	{
		if (g_theInput->IsKeyDown('A'))
		{
			PlayAnimation("TurnLeft");
			horizontalVelocity += left * playerStrafeSpeed;
			m_isTurning = true;
		}
		else if (g_theInput->IsKeyDown('D'))
		{
			PlayAnimation("TurnRight");
			horizontalVelocity -= left * playerStrafeSpeed;
			m_isTurning = true;
		}
		else
		{
			if (m_isTurning)
			{
				PlayAnimation("Walk");
				m_isTurning = false;
			}
		}
	}

	m_velocity = Vec3(horizontalVelocity.x, horizontalVelocity.y, m_velocity.z);
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);
}

void Player::Respawn()
{
	m_position = Vec3::ZERO;
	m_velocity = Vec3::ZERO;
	m_orientation = EulerAngles::ZERO;
	m_isGrounded = false;
}

void Player::PlayAnimation(std::string const& name)
{
	for (int animIndex = 0; animIndex < static_cast<int>(m_playerDef->m_animationGroups.size()); ++animIndex)
	{
		if (m_playerDef->m_animationGroups[animIndex]->m_animationGroupName.compare(name) == 0)
		{
			if (m_animGroup != m_playerDef->m_animationGroups[animIndex])
			{
				m_animGroup = m_playerDef->m_animationGroups[animIndex];
				m_animationClock->Reset();
			}
			break;
		}
	}
}
