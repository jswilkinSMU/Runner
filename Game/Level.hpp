#pragma once
#include "Game/GameCommon.h"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <vector>
// -----------------------------------------------------------------------------
struct LevelDefinition;
class Player;
class VertexBuffer;
class IndexBuffer;
class Shader;
//------------------------------------------------------------------------------
struct Block
{
	//AABB3 m_bounds = AABB3(Vec3::ZERO, Vec3::ZERO);
	OBB3  m_bounds = OBB3(Vec3::ZERO, Vec3::ZERO, Vec3::ZERO, Vec3::ZERO, Vec3::ZERO);
	Rgba8 m_blockColor = Rgba8::WHITE;
	EulerAngles m_blockOrientation = EulerAngles::ZERO;
};
// -----------------------------------------------------------------------------
struct EndGoal
{
	EndGoal(Vec3 center, float radius, Rgba8 color)
		:m_center(center), m_radius(radius), m_endGoalColor(color) {}
	Vec3  m_center = Vec3::ZERO;
	float m_radius = 0.0f;
	Rgba8 m_endGoalColor = Rgba8::WHITE;
};
// -----------------------------------------------------------------------------
class Level
{
public:

	Level(Game* owner, LevelDefinition* levelDef);
	~Level();

	void CreateLevelGeometry();
	void CreateBuffers();

	void LayoutLevelsFromDefinitions(LevelDefinition* levelDef);
	void SpawnBlock(Vec3 center, Vec3 dimensions, EulerAngles blockOrientation, Rgba8 color);
	void SpawnEndGoal(Vec3 center, float radius, Rgba8 color);

	void Update(float deltaSeconds);

	void Render() const;
	void DrawLevelItems() const;

	void ClearBuffers();
	void DestroyGeometry();

	void CollidePlayerWithBlocks();
	void CollidePlayerWithBlocks(Player* playerCharacter);
	void CheckDeathBounds(Player* playerCharacter);
	void CheckPlayerVsEndGoal(Player* playerCharacter);
	void AdvanceToNextLevel();
	bool RaycastDown(Vec3 const& rayStartPos, float maxDist, Vec3& impactPos);

private:
	Game* m_theGame = nullptr;
	LevelDefinition* m_levelDef = nullptr;
	Shader* m_phongShader = nullptr;
	Vec3   m_sunDirection = Vec3(3.f, 0.f, 2.f);
	float  m_sunIntensity = 0.75f;
	float  m_ambientIntensity = 0.35f;

	std::vector<Vertex_PCUTBN> m_blockTBNVerts;
	std::vector<unsigned int> m_blockIndices;
	VertexBuffer* m_blockVBO = nullptr;
	IndexBuffer* m_blockIBO = nullptr;

	EndGoal* m_endGoal = nullptr;
	std::vector<Block*> m_blocks;
	AABB3 m_deathBounds = AABB3(Vec3(-20.f, -20.f, -200.f), Vec3(1000.f, 1000.f, -20.f));
	bool m_isLevelComplete = false;
};