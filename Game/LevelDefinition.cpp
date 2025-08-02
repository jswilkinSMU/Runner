#include "Game/LevelDefinition.hpp"
#include "Game/GameCommon.h"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.h"
// -----------------------------------------------------------------------------
std::vector<LevelDefinition*> LevelDefinition::s_levelDefinitions;
// -----------------------------------------------------------------------------
SpawnInfo::SpawnInfo(XmlElement const& spawnElement)
{
	m_levelItem = ParseXmlAttribute(spawnElement, "levelItem", m_levelItem);
	m_center = ParseXmlAttribute(spawnElement, "center", m_center);
	m_dimensions = ParseXmlAttribute(spawnElement, "dimensions", m_dimensions);
	m_orientation = ParseXmlAttribute(spawnElement, "orientation", m_orientation);
	m_radius = ParseXmlAttribute(spawnElement, "radius", m_radius);
	m_color = ParseXmlAttribute(spawnElement, "color", m_color);
}
// -----------------------------------------------------------------------------
LevelDefinition::LevelDefinition(XmlElement const& levelDefElement)
{
	// Parsing name
	m_levelName = ParseXmlAttribute(levelDefElement, "name", m_levelName);

	// Parsing shader
	std::string shader = ParseXmlAttribute(levelDefElement, "shader", shader);
	if (shader == "Default")
	{
		m_shader = nullptr;
	}
	else
	{
		m_shader = g_theRenderer->CreateShader(shader.c_str(), VertexType::VERTEX_PCUTBN);
	}

	// Parsing spawn info
	XmlElement const* spawnInfosElement = levelDefElement.FirstChildElement("SpawnInfos");
	if (spawnInfosElement)
	{
		for (XmlElement const* spawnInfoElement = spawnInfosElement->FirstChildElement("SpawnInfo");
			spawnInfoElement != nullptr; spawnInfoElement = spawnInfoElement->NextSiblingElement("SpawnInfo"))
		{
			m_itemSpawnInfo.push_back(SpawnInfo(*spawnInfoElement));
		}
	}
}

void LevelDefinition::InitializeLevelDefinitions()
{
	XmlDocument levelDefsXml;
	char const* filePath = "Data/Definitions/LevelDefinitions.xml";
	XmlError result = levelDefsXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open required level definitions file \"%s\"", filePath));

	XmlElement* rootElement = levelDefsXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "RootElement not found!");

	XmlElement* levelDefElement = rootElement->FirstChildElement();
	while (levelDefElement)
	{
		std::string elementName = levelDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "LevelDefinition", Stringf("Root child element in %s was <%s>, must be <LevelDefinition>!", filePath, elementName.c_str()));
		LevelDefinition* newLevelDef = new LevelDefinition(*levelDefElement);
		s_levelDefinitions.push_back(newLevelDef);
		levelDefElement = levelDefElement->NextSiblingElement();
	}
}

void LevelDefinition::ClearLevelDefinitions()
{
	s_levelDefinitions.clear();
}

LevelDefinition* LevelDefinition::GetLevelByName(std::string const& name)
{
	for (int levelDefIndex = 0; levelDefIndex < static_cast<int>(s_levelDefinitions.size()); ++levelDefIndex)
	{
		if (s_levelDefinitions[levelDefIndex]->m_levelName == name)
		{
			return s_levelDefinitions[levelDefIndex];
		}
	}
	return nullptr;
}
