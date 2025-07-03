#include "Game/PlayerDefinition.hpp"
#include "Game/AnimationGroup.hpp"
#include "Game/GameCommon.h"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.h"

std::vector<PlayerDefinition*> PlayerDefinition::s_playerDefs;

PlayerDefinition::PlayerDefinition(XmlElement const& playerDefElement)
{
	m_playerName = ParseXmlAttribute(playerDefElement, "name", m_playerName);
	m_isVisible = ParseXmlAttribute(playerDefElement, "visible", m_isVisible);

	ParseCollision(playerDefElement);
	ParsePhysics(playerDefElement);
	ParseCamera(playerDefElement);
	ParseVisuals(playerDefElement);
}

void PlayerDefinition::ParseCollision(XmlElement const& playerDefElement)
{
	XmlElement const* collisionElement = playerDefElement.FirstChildElement("Collision");
	if (!collisionElement)
	{
		return;
	}

	m_physicsRadius		 = ParseXmlAttribute(*collisionElement, "physicsRadius", m_physicsRadius);
	m_physicsHeight		 = ParseXmlAttribute(*collisionElement, "physicsHeight", m_physicsHeight);
	m_collidesWithBlocks = ParseXmlAttribute(*collisionElement, "collidesWithBlock", m_collidesWithBlocks);
}

void PlayerDefinition::ParsePhysics(XmlElement const& playerDefElement)
{
	XmlElement const* physicsElement = playerDefElement.FirstChildElement("Physics");
	if (!physicsElement)
	{
		return;
	}

	m_isSimulated = ParseXmlAttribute(*physicsElement, "simulated", m_isSimulated);
	m_moveSpeed	  = ParseXmlAttribute(*physicsElement, "moveSpeed", m_moveSpeed);
	m_strafeSpeed = ParseXmlAttribute(*physicsElement, "strafeSpeed", m_strafeSpeed);
	m_jumpForce   = ParseXmlAttribute(*physicsElement, "jumpForce", m_jumpForce);
}

void PlayerDefinition::ParseCamera(XmlElement const& playerDefElement)
{
	XmlElement const* cameraElement = playerDefElement.FirstChildElement("Camera");
	if (!cameraElement)
	{
		return;
	}

	m_cameraFOV = ParseXmlAttribute(*cameraElement, "cameraFOV", m_cameraFOV);
}

void PlayerDefinition::ParseVisuals(XmlElement const& playerDefElement)
{
	XmlElement const* visualElement = playerDefElement.FirstChildElement("Visuals");
	if (!visualElement)
	{
		return;
	}

	m_spriteSize = ParseXmlAttribute(*visualElement, "spriteSize", m_spriteSize);
	m_spritePivot = ParseXmlAttribute(*visualElement, "spritePivot", m_spritePivot);

	std::string billboardType = ParseXmlAttribute(*visualElement, "billboardType", billboardType);
	if (billboardType == "WorldUpFacing")
	{
		m_billboardType = BillboardType::WORLD_UP_FACING;
	}
	else if (billboardType == "WorldUpOpposing")
	{
		m_billboardType = BillboardType::WORLD_UP_OPPOSING;
	}
	else if (billboardType == "FullOpposing")
	{
		m_billboardType = BillboardType::FULL_OPPOSING;
	}

	m_renderLit = ParseXmlAttribute(*visualElement, "renderLit", m_renderLit);
	m_renderRounded = ParseXmlAttribute(*visualElement, "renderRounded", m_renderRounded);

	std::string shader = ParseXmlAttribute(*visualElement, "shader", shader);
	if (shader == "Default")
	{
		m_shader = nullptr;
	}
	else
	{
		m_shader = g_theRenderer->CreateShader(shader.c_str(), VertexType::VERTEX_PCUTBN);
	}

	std::string spritesheet = ParseXmlAttribute(*visualElement, "spriteSheet", spritesheet);
	m_cellCount = ParseXmlAttribute(*visualElement, "cellCount", m_cellCount);
	Texture* spriteSheetTextureImg = g_theRenderer->CreateOrGetTextureFromFile(spritesheet.c_str());
	m_spriteSheet = new SpriteSheet(*spriteSheetTextureImg, m_cellCount);

	XmlElement const* animGroupElement = visualElement->FirstChildElement("AnimationGroup");
	while (animGroupElement != nullptr)
	{
		AnimationGroup* animGroup = new AnimationGroup(*animGroupElement, m_spriteSheet);
		m_animationGroups.push_back(animGroup);
		animGroupElement = animGroupElement->NextSiblingElement("AnimationGroup");
	}
}

void PlayerDefinition::InitializePlayerDefintions()
{
	XmlDocument playerDefsXml;
	char const* filePath = "Data/Definitions/PlayerDefinitions.xml";
	XmlError result = playerDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required player definitions file \"s\"", filePath));

	XmlElement* rootElement = playerDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* playerDefElement = rootElement->FirstChildElement();
	while (playerDefElement)
	{
		std::string elementName = playerDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "PlayerDefinition", Stringf("Root child element in %s was <%s>, must be <PlayerDefinitions>!", filePath, elementName.c_str()));
		PlayerDefinition* newPlayerDefinition = new PlayerDefinition(*playerDefElement);
		s_playerDefs.push_back(newPlayerDefinition);
		playerDefElement = playerDefElement->NextSiblingElement();
	}
}

void PlayerDefinition::ClearPlayerDefinitions()
{
	for (int playerDefIndex = 0; playerDefIndex < static_cast<int>(s_playerDefs.size()); ++playerDefIndex)
	{
		delete s_playerDefs[playerDefIndex];
		s_playerDefs[playerDefIndex] = nullptr;
	}
	s_playerDefs.clear();
}

PlayerDefinition* PlayerDefinition::GetPlayerByName(std::string const& playerName)
{
	for (int playerDefIndex = 0; playerDefIndex < static_cast<int>(s_playerDefs.size()); ++playerDefIndex)
	{
		if (s_playerDefs[playerDefIndex]->m_playerName == playerName)
		{
			return s_playerDefs[playerDefIndex];
		}
	}
	return nullptr;
}

AnimationGroup* PlayerDefinition::GetAnimationByName(std::string const& animationName)
{
	for (int animDefIndex = 0; animDefIndex < static_cast<int>(m_animationGroups.size()); ++animDefIndex)
	{
		if (m_animationGroups[animDefIndex]->m_animationGroupName == animationName)
		{
			return m_animationGroups[animDefIndex];
		}
	}
	return nullptr;
}
