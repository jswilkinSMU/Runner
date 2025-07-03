#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/MathUtils.h"
#include <vector>
// -----------------------------------------------------------------------------
class AnimationGroup;
class Shader;
class SpriteSheet;
// -----------------------------------------------------------------------------
struct PlayerDefinition
{
	PlayerDefinition(XmlElement const& playerDefElement);
	static std::vector<PlayerDefinition*> s_playerDefs;
	static void InitializePlayerDefintions();
	static void ClearPlayerDefinitions();
	static PlayerDefinition* GetPlayerByName(std::string const& playerName);
	AnimationGroup* GetAnimationByName(std::string const& animationName);
// -----------------------------------------------------------------------------
	void ParseCollision(XmlElement const& playerDefElement);
	void ParsePhysics(XmlElement const& playerDefElement);
	void ParseCamera(XmlElement const& playerDefElement);
	void ParseVisuals(XmlElement const& playerDefElement);
// -----------------------------------------------------------------------------
	std::string m_playerName		 = "default";
	bool		m_isVisible			 = false;
	float		m_physicsRadius		 = 0.0f;
	float		m_physicsHeight		 = 0.0f;
	bool		m_collidesWithBlocks = false;
	bool		m_isSimulated		 = false;
	float		m_moveSpeed			 = 0.0f;
	float		m_strafeSpeed		 = 0.0f;
	float		m_jumpForce			 = 0.0f;
	float		m_cameraFOV			 = 0.0f;
// -----------------------------------------------------------------------------
	Vec2		  m_spriteSize = Vec2::ONE;
	Vec2          m_spritePivot = Vec2::ONEHALF;
	BillboardType m_billboardType = BillboardType::NONE;
	bool          m_renderLit = false;
	bool		  m_renderRounded = false;
	Shader* m_shader = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;
	IntVec2       m_cellCount = IntVec2::ONE;
	Vec3		  m_direction = Vec3::XAXE;
	int			  m_startFrame = 0;
	int			  m_endFrame = 0;
	std::vector<AnimationGroup*> m_animationGroups;
};