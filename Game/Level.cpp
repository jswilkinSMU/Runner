#include "Game/Level.hpp"
#include "Game/LevelDefinition.hpp"
#include "Game/Player.hpp"
#include "Game/Game.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/RaycastUtils.hpp"

Level::Level(Game* owner, LevelDefinition* levelDef)
	:m_theGame(owner),
	 m_levelDef(levelDef)
{
	m_phongShader = g_theRenderer->CreateOrGetShader("Data/Shaders/Phong", VertexType::VERTEX_PCUTBN);
	LayoutLevelsFromDefinitions(m_levelDef);
	CreateLevelGeometry();
	CreateBuffers();
}

Level::~Level()
{
	ClearBuffers();
	DestroyGeometry();
}

void Level::CreateLevelGeometry()
{
	// Blocks
	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block const* block = m_blocks[blockIndex];
		AddVertsForOBB3D(m_blockTBNVerts, m_blockIndices, block->m_bounds, block->m_blockColor);
	}

	// End Goal
	AddVertsForSphere3D(m_blockTBNVerts, m_blockIndices, m_endGoal->m_center, m_endGoal->m_radius, m_endGoal->m_endGoalColor);
}

void Level::CreateBuffers()
{
	// Create buffers and copy to GPU
	m_blockVBO = g_theRenderer->CreateVertexBuffer(static_cast<unsigned int>(m_blockTBNVerts.size()) * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_blockIBO = g_theRenderer->CreateIndexBuffer(static_cast<unsigned int>(m_blockIndices.size()) * sizeof(unsigned int), sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_blockTBNVerts.data(), m_blockVBO->GetSize(), m_blockVBO);
	g_theRenderer->CopyCPUToGPU(m_blockIndices.data(), m_blockIBO->GetSize(), m_blockIBO);
}

void Level::LayoutLevelsFromDefinitions(LevelDefinition* levelDef)
{
	for (SpawnInfo const& spawnInfo : levelDef->m_itemSpawnInfo)
	{
		if (spawnInfo.m_levelItem == "Block")
		{
			SpawnBlock(spawnInfo.m_center, spawnInfo.m_dimensions, spawnInfo.m_orientation, spawnInfo.m_color);
		}
		else if (spawnInfo.m_levelItem == "EndGoal")
		{
			SpawnEndGoal(spawnInfo.m_center, spawnInfo.m_radius, spawnInfo.m_color);
		}
	}
}

void Level::SpawnBlock(Vec3 center, Vec3 dimensions, EulerAngles blockOrientation, Rgba8 color)
{
	// AABB3
	//Vec3 halfDims = dimensions * 0.5f;
	//EulerAngles orientation = EulerAngles::ZERO;
	//AABB3 bounds(center - halfDims, center + halfDims);
	//m_blocks.push_back(new Block{ bounds, color });

	// OBB3
	Vec3 halfDims = dimensions * 0.5f;

	Mat44 rotationMat = blockOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	Vec3 iBasis = rotationMat.GetIBasis3D();
	Vec3 jBasis = rotationMat.GetJBasis3D();
	Vec3 kBasis = rotationMat.GetKBasis3D();

	OBB3 bounds(center, iBasis, jBasis, kBasis, halfDims);
	m_blocks.push_back(new Block{ bounds, color, blockOrientation });
}

void Level::SpawnEndGoal(Vec3 center, float radius, Rgba8 color)
{
	m_endGoal = new EndGoal(center, radius, color);
}

void Level::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theGame->m_player != nullptr && g_theGame->GetCurrentGameState() == GameState::LEVEL_PLAYING)
	{
		CollidePlayerWithBlocks();
		CheckDeathBounds(g_theGame->m_player);
		CheckPlayerVsEndGoal(g_theGame->m_player);
	}
}

void Level::Render() const
{
	DrawLevelItems();
}

void Level::DrawLevelItems() const
{
	g_theRenderer->SetLightingConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindSampler(SamplerMode::POINT_CLAMP, 0);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 1);
	g_theRenderer->BindSampler(SamplerMode::BILINEAR_WRAP, 2);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(m_phongShader);
	g_theRenderer->DrawIndexedVertexBuffer(m_blockVBO, m_blockIBO, static_cast<unsigned int>(m_blockIndices.size()));
}

void Level::ClearBuffers()
{
	delete m_blockVBO;
	m_blockVBO = nullptr;

	delete m_blockIBO;
	m_blockIBO = nullptr;
}

void Level::DestroyGeometry()
{
	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block*& block = m_blocks[blockIndex];
		if (block != nullptr)
		{
			delete block;
			block = nullptr;
		}
	}

	delete m_endGoal;
	m_endGoal = nullptr;
}

void Level::CollidePlayerWithBlocks()
{
	if (g_theGame->m_player != nullptr)
	{
		CollidePlayerWithBlocks(g_theGame->m_player);
	}
}

void Level::CollidePlayerWithBlocks(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;
	playerCharacter->m_isGrounded = false;

	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block*& block = m_blocks[blockIndex];
		Vec3 blockCenter = block->m_bounds.m_center;
		Vec3 halfDims = block->m_bounds.m_halfDimensions;
		AABB3 alignedBox = AABB3(blockCenter - halfDims, blockCenter + halfDims);

		if (PushZCylinderOutOfFixedAABB3D(playerPos, radius, height, alignedBox))
		{
			float playerBottomZ = playerPos.z - (height * 0.5f);
			float blockTopZ = alignedBox.m_maxs.z;

			if (fabsf(playerBottomZ - blockTopZ) < 0.05f)
			{
				playerCharacter->m_isGrounded = true;
				playerCharacter->m_velocity.z = 0.f;
			}

			Vec3 alignedblockCenter = alignedBox.GetCenter();
			Vec3 pushDirection = (playerPos - alignedblockCenter).GetNormalized();
			Vec3 horizontalVelocity = Vec3(playerCharacter->m_velocity.x, playerCharacter->m_velocity.y, 0.f);

			float pushAmount = DotProduct3D(playerCharacter->m_velocity, pushDirection);
			if (pushAmount > 0.f)
			{
				Vec3 pushVelocity = pushAmount * pushDirection;
				playerCharacter->m_velocity.x -= pushVelocity.x;
				playerCharacter->m_velocity.y -= pushVelocity.y;
			}
		}
	}
}

void Level::CheckDeathBounds(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;

	if (DoZCylinderAndAABB3Overlap3D(playerPos, radius, height, m_deathBounds))
	{
		playerCharacter->Respawn();
	}
}

void Level::CheckPlayerVsEndGoal(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;

	if (DoZCylinderAndSphereOverlap3D(playerPos, radius, height, m_endGoal->m_center, m_endGoal->m_radius))
	{
		m_isLevelComplete = true;
		AdvanceToNextLevel();
	}
}

void Level::AdvanceToNextLevel()
{
	if (m_isLevelComplete)
	{
		g_theGame->m_currentLevelIndex++;

		if (g_theGame->m_currentLevelIndex < static_cast<int>(g_theGame->m_levels.size()))
		{
			g_theGame->LoadNextLevel();
		}
		else
		{
			g_theGame->EnterState(GameState::GAME_COMPLETE);
		}
	}
}

bool Level::RaycastDown(Vec3 const& rayStartPos, float maxDist, Vec3& impactPos)
{
	Vec3 direction = -Vec3::ZAXE;

	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block*& block = m_blocks[blockIndex];
		RaycastResult3D raycastResult;
		raycastResult.m_rayStartPosition = rayStartPos;
		raycastResult.m_rayFwdNormal = direction;
		
		raycastResult = RaycastVsOBB3D(rayStartPos, direction, maxDist, block->m_bounds);

		if (raycastResult.m_didImpact)
		{
			impactPos = raycastResult.m_impactPos;
			return true;
		}
	}

	return false;
}
