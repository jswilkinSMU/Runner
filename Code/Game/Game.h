#pragma once
#include "Game/GameCommon.h"
#include "Game/Entity.hpp"
#include "Engine/Renderer/Camera.h"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include <string>
// -----------------------------------------------------------------------------
class Player;
class Texture;
class Shader;
class BitmapFont;
class VertexBuffer;
class IndexBuffer;
//------------------------------------------------------------------------------
struct Block
{
	AABB3 m_bounds = AABB3(Vec3::ZERO, Vec3::ZERO);
	//OBB3  m_bounds = OBB3(Vec3::ZERO, Vec3::ZERO, Vec3::ZERO, Vec3::ZERO, Vec3::ZERO);
	Rgba8 m_blockColor = Rgba8::WHITE;
	EulerAngles m_blockOrientation = EulerAngles::ZERO;

	// TODO: Change to OBB3 so we can rotate blocks on side collision
	//Vec3 m_center = Vec3::ZERO;
	//Vec3 m_iBasis = Vec3::ZERO;
	//Vec3 m_jBasis = Vec3::ZERO;
	//Vec3 m_kBasis = Vec3::ZERO;
	//Vec3 m_halfDimensions = Vec3::ZERO;
};
struct EndGoal
{
	EndGoal(Vec3 center, float radius, Rgba8 color)
		:m_center(center), m_radius(radius), m_endGoalColor(color){}
	Vec3  m_center = Vec3::ZERO;
	float m_radius = 0.0f;
	Rgba8 m_endGoalColor = Rgba8::WHITE;
};
// -----------------------------------------------------------------------------
class Game
{
public:
	App* m_app;
	Game(App* owner);
	~Game();
	void StartUp();
	void CreateLevelGeometry();
	void CreateBuffers();

	void LayoutLevelOne();
	void InitializePlayer();
	void SpawnBlock(Vec3 center, Vec3 dimensions, Rgba8 color);

	void SetupUIMainMenu();
	void SetupUILevelSelect();
	void SetupUIControls();

	void Update();
	void UpdateCameras(float deltaSeconds);
	void FreeFlyControls(float deltaSeconds);

	void Render() const;
	void RenderMainMenu() const;
	void RenderLevelSelect() const;
	void RenderControls() const;
	void DrawLevelItems() const;
	void DrawBackgroundTexture() const;

	void Shutdown();
	void ClearBuffers();
	void DestroyGeometry();
	void DestroyPlayer();

	void KeyInputPresses();
	void AdjustForPauseAndTimeDistortion(float deltaSeconds);
	void HandleCameraInput();
	void UpdateUIPresses(float deltaSeconds);

	void CollidePlayerWithBlocks();
	void CollidePlayerWithBlocks(Player* playerCharacter);
	void CheckDeathBounds(Player* playerCharacter);
	void CheckPlayerVsEndGoal(Player* playerCharacter);

	GameState GetCurrentGameState() const;
	void EnterState(GameState state);
	void ExitState(GameState state);

private:
	Camera		m_screenCamera;
	Camera      m_gameWorldCamera;
	Clock		m_gameClock;
	GameState   m_currentGameState = GameState::MAIN_MENU;
	CameraState m_currentCameraState = CameraState::PLAYER_FOLLOW;
	Texture*    m_backgroundTexture = nullptr;
	Shader*		m_phongShader = nullptr;

	Vec3 m_cameraPosition = Vec3::ZERO;
	EulerAngles m_cameraOrientation = EulerAngles::ZERO;
	Vec3 m_sunDirection = Vec3(3.f, 0.f, 2.f);
	float m_sunIntensity = 0.75f;
	float m_ambientIntensity = 0.35f;

	Player* m_player = nullptr;
	EndGoal* m_endGoal = nullptr;
	std::vector<Vertex_PCUTBN> m_blockTBNVerts;
	std::vector<unsigned int> m_blockIndices;
	VertexBuffer* m_blockVBO = nullptr;
	IndexBuffer* m_blockIBO = nullptr;
	std::vector<Block*> m_blocks;
	AABB3 m_deathBounds = AABB3(Vec3(-20.f, -20.f, -200.f), Vec3(1000.f, 1000.f, -20.f));

	// UI
	AABB2 m_startButtonBounds = AABB2(600.f, 400.f, 1000.f, 460.f);
	AABB2 m_exitButtonBounds = AABB2(600.f, 300.f, 1000.f, 360.f);
	AABB2 m_controlsButtonBounds = AABB2(200.f, 100.f, 400.f, 160.f);
	BitmapFont* m_font = nullptr;

	// Music
	std::string m_gameMusicPath;
	std::string m_clickSoundPath;
	SoundID     m_gameMusic;
	SoundID     m_clickSound;
	SoundPlaybackID m_gameMusicPlayback;
	float m_musicVolume = 0.0f;
};