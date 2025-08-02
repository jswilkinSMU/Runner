#pragma once
#include "Game/GameCommon.h"
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
class Level;
class Texture;
class BitmapFont;
// -----------------------------------------------------------------------------
class Game
{
public:
	App* m_app;
	Game(App* owner);
	~Game();
	void StartUp();
	void InitializeRunner();
	void InitializeSkater();
	void InitializeLevels();

	void SetupUIMainMenu();
	void SetupUILevelSelect();
	void SetupUIControls();
	void SetupUICharacterSelect();
	void SetupCredits();
	void CreateLevelButton(std::string const& buttonName, int levelIndex, AABB2 buttonBounds);
	void ToggleDebugText();

	void Update();
	void LoadNextLevel();
	void ToggleUnlockMode();
	void UpdateCameras(float deltaSeconds);
	void FreeFlyControls(float deltaSeconds);

	void Render() const;
	void RenderMainMenu() const;
	void RenderLevelSelect() const;
	void RenderCharacterSelect() const;
	void RenderControls() const;
	void RenderCredits() const;
	void RenderGameComplete() const;
	void DrawBackgroundTexture() const;

	void Shutdown();
	void DestroyPlayer();
	void DestroyLevel();

	void KeyInputPresses();
	void AdjustForPauseAndTimeDistortion(float deltaSeconds);
	void HandleCameraInput();
	void UpdateUIPresses(float deltaSeconds);

	GameState GetCurrentGameState() const;
	void EnterState(GameState state);
	void ExitState(GameState state);

public:
	Clock* m_gameClock = nullptr;
	Camera m_gameWorldCamera;
	Player* m_player = nullptr;
	Level*  m_currentLevel = nullptr;
	int m_currentLevelIndex = 0;
	std::vector<Level*> m_levels;

private:
	GameState   m_currentGameState = GameState::MAIN_MENU;
	Texture*    m_backgroundTexture = nullptr;

	// Level Locking
	bool m_isUnlockMode = false;
	std::vector<bool> m_levelsUnlocked;

	// Camera
	Camera		m_screenCamera;
	CameraState m_currentCameraState = CameraState::PLAYER_FOLLOW;
	Vec3 m_cameraPosition = Vec3::ZERO;
	EulerAngles m_cameraOrientation = EulerAngles::ZERO;

	// UI
	AABB2 m_controlsButtonBounds = AABB2(200.f, 100.f, 400.f, 160.f);
	BitmapFont* m_font = nullptr;
	bool m_isDebugTextOn = false;

	// Music
	std::string m_gameMusicPath;
	std::string m_clickSoundPath;
	SoundID     m_gameMusic;
	SoundID     m_clickSound;
	SoundPlaybackID m_gameMusicPlayback;
	float m_musicVolume = 0.0f;
};