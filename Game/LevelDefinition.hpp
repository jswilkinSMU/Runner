#pragma once
#include "Engine/Core/XmlUtils.hpp"
// -----------------------------------------------------------------------------
class Shader;
// -----------------------------------------------------------------------------
struct SpawnInfo
{
	SpawnInfo(XmlElement const& spawnElement);
	
	std::string m_levelItem = "default";
	Vec3 m_center = Vec3::ZERO;
	Vec3 m_dimensions = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	float m_radius = 0.0f;
	Rgba8 m_color = Rgba8::WHITE;
};
// -----------------------------------------------------------------------------
struct LevelDefinition
{
	LevelDefinition(XmlElement const& levelDefElement);
	static std::vector<LevelDefinition*> s_levelDefinitions;
	static void InitializeLevelDefinitions();
	static void ClearLevelDefinitions();
	static LevelDefinition* GetLevelByName(std::string const& name);
// -----------------------------------------------------------------------------
	std::string m_levelName = "default";
	Shader* m_shader = nullptr;
	std::vector<SpawnInfo> m_itemSpawnInfo;
};
// -----------------------------------------------------------------------------