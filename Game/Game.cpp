#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/App.h"
#include "Game/PlayerDefinition.hpp"
#include "Game/Player.hpp"
#include "Game/Level.hpp"
#include "Game/LevelDefinition.hpp"

#include "Engine/Input/InputSystem.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Window/Window.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Rgba8.h"
#include "Engine/Core/Vertex_PCU.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/VertexUtils.h"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.h"
#include "Engine/Math/AABB3.hpp"
#include "Engine/UI/UISystem.hpp"
#include "Engine/UI/Elements/UIButton.hpp"
#include "Engine/UI/Elements/UIBorder.hpp"

Game::Game(App* owner)
	: m_app(owner)
{
	// Get sound from game config
	m_gameMusicPath = g_gameConfigBlackboard.GetValue("gameMusic", "default");
	m_clickSoundPath = g_gameConfigBlackboard.GetValue("buttonClickSound", "default");
	m_musicVolume = g_gameConfigBlackboard.GetValue("musicVolume", 0.f);

	m_gameMusic = g_theAudio->CreateOrGetSound(m_gameMusicPath);
	m_clickSound = g_theAudio->CreateOrGetSound(m_clickSoundPath);
	m_gameMusicPlayback = m_gameMusic;
}

Game::~Game()
{
}

void Game::StartUp()
{
	// Write control interface into devconsole
	g_theDevConsole->AddLine(Rgba8::CYAN, "Welcome to Runner!");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");
	g_theDevConsole->AddLine(Rgba8::CYAN, "CONTROLS:");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "ESC   - Quits the game");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "SPACE	- Start game and Jump");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "R     - Reset position and orientation back to start.");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "P     - Pauses the game");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "A/D   - Move left/right");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "LMB   - Presses buttons");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F1    - Toggle player physics cylinder");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F2    - Toggle debug text for time and FPS");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F4    - Toggle camera switch");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "K     - Toggle unlock mode (unlocks all levels for testing)");
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "L     - Toggle planar shadow on/off");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");

	m_backgroundTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/galaxy.jpg");
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");

	EnterState(GameState::MAIN_MENU);
	m_gameClock = new Clock(Clock::GetSystemClock());

	PlayerDefinition::InitializePlayerDefintions();
	LevelDefinition::InitializeLevelDefinitions();

	InitializeLevels();
}

void Game::InitializeRunner()
{
	m_player = new Player(this, Vec3::ZERO, EulerAngles::ZERO, Rgba8(0, 0, 0, 180), PlayerDefinition::GetPlayerByName("Runner"));
}

void Game::InitializeSkater()
{
	m_player = new Player(this, Vec3::ZERO, EulerAngles::ZERO, Rgba8(0, 0, 0, 180), PlayerDefinition::GetPlayerByName("Skater"));
}

void Game::InitializeLevels()
{
	m_levels.push_back(new Level(this, LevelDefinition::GetLevelByName("LevelOne")));
	m_levels.push_back(new Level(this, LevelDefinition::GetLevelByName("LevelTwo")));
	m_levels.push_back(new Level(this, LevelDefinition::GetLevelByName("LevelThree")));
	m_levels.push_back(new Level(this, LevelDefinition::GetLevelByName("LevelFour")));
	m_levels.push_back(new Level(this, LevelDefinition::GetLevelByName("LevelFive")));

	m_levelsUnlocked = { true, false, false, false, false };
}

void Game::SetupUIMainMenu()
{
	AABB2 startButtonBounds = AABB2(600.f, 400.f, 1000.f, 460.f);
	AABB2 exitButtonBounds = AABB2(600.f, 300.f, 1000.f, 360.f);

	UIButton* startButton = new UIButton("StartButton", startButtonBounds, "Start", m_font);
	startButton->SetButtonBackgroundColor(Rgba8::SEAWEED);
	startButton->SetButtonHoverColor(Rgba8(20, 60, 20, 120));
	startButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::CHARACTER_SELECT);
	});
	g_theUISystem->AddElement(startButton);

	UIBorder* startBorder = new UIBorder("StartButtonBorder", startButtonBounds, Rgba8::BLACK, 2.5f);
	g_theUISystem->AddElement(startBorder);

	UIButton* exitButton = new UIButton("ExitButton", exitButtonBounds, "Exit", m_font);
	exitButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	exitButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	exitButton->SetOnClickCallback([]()
	{
		g_theEventSystem->FireEvent("Quit");
	});
	g_theUISystem->AddElement(exitButton);

	UIBorder* exitBorder = new UIBorder("ExitButtonBorder", exitButtonBounds, Rgba8::BLACK, 2.5f);
	g_theUISystem->AddElement(exitBorder);

	UIButton* controlsButton = new UIButton("ControlsButton", m_controlsButtonBounds, "Controls", m_font);
	controlsButton->SetButtonBackgroundColor(Rgba8::SEAWEED);
	controlsButton->SetButtonHoverColor(Rgba8(20, 60, 20, 120));
	controlsButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::CONTROLS);
	});
	g_theUISystem->AddElement(controlsButton);

	UIBorder* controlsBorder = new UIBorder("ControlsButtonBorder", m_controlsButtonBounds, Rgba8::BLACK, 2.5f);
	g_theUISystem->AddElement(controlsBorder);

	AABB2 creditBounds = AABB2(1200.f, 100.f, 1400.f, 160.f);

	UIButton* creditsButton = new UIButton("CreditsButton", creditBounds, "Credits", m_font);
	creditsButton->SetButtonBackgroundColor(Rgba8::SEAWEED);
	creditsButton->SetButtonHoverColor(Rgba8(20, 60, 20, 120));
	creditsButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::CREDITS);
	});
	g_theUISystem->AddElement(creditsButton);

	UIBorder* creditsBorder = new UIBorder("CreditsBorder", creditBounds, Rgba8::BLACK, 2.f);
	g_theUISystem->AddElement(creditsBorder);
}

void Game::SetupUILevelSelect()
{
	AABB2 levelOneButtonBounds = AABB2(600.f, 460.f, 1000.f, 520.f);
	AABB2 levelTwoButtonBounds = AABB2(600.f, 380.f, 1000.f, 440.f);
	AABB2 levelThreeButtonBounds = AABB2(600.f, 300.f, 1000.f, 360.f);
	AABB2 levelFourButtonBounds = AABB2(600.f, 220.f, 1000.f, 280.f);
	AABB2 levelFiveButtonBounds = AABB2(600.f, 140.f, 1000.f, 200.f);
	AABB2 backButtonBounds = AABB2(600.f, 60.f, 1000.f, 120.f);

	CreateLevelButton("LevelOneButton", 0, levelOneButtonBounds);
	CreateLevelButton("LevelTwoButton", 1, levelTwoButtonBounds);
	CreateLevelButton("LevelThreeButton", 2, levelThreeButtonBounds);
	CreateLevelButton("LevelFourButton", 3, levelFourButtonBounds);
	CreateLevelButton("LevelFiveButton", 4, levelFiveButtonBounds);

	UIButton* backButton = new UIButton("BackButton", backButtonBounds, "Back", m_font);
	backButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	backButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	backButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		DestroyPlayer();
		EnterState(GameState::CHARACTER_SELECT);
	});
	g_theUISystem->AddElement(backButton);
}

void Game::SetupUIControls()
{
	UIButton* backButton = new UIButton("BackButton", m_controlsButtonBounds, "Back", m_font);
	backButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	backButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	backButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::MAIN_MENU);
	});
	g_theUISystem->AddElement(backButton);

	UIBorder* controlsBorder = new UIBorder("ControlsBorder", AABB2(Vec2(500.f, 650.f), Vec2(1100.f, 150.f)), Rgba8::LIMEGREEN, 2.f);
	controlsBorder->SetBorderStyle(BorderStyle::DASHED);
	controlsBorder->SetGapLength(7.f);
	controlsBorder->SetDashLength(10.f);
	g_theUISystem->AddElement(controlsBorder);

	UIBorder* backBorder = new UIBorder("BackButtonBorder", m_controlsButtonBounds, Rgba8::BLACK, 5.5f);
	g_theUISystem->AddElement(backBorder);
}

void Game::SetupUICharacterSelect()
{
	AABB2 runnerButtonBounds = AABB2(600.f, 400.f, 1000.f, 460.f);
	AABB2 skaterButtonBounds = AABB2(600.f, 300.f, 1000.f, 360.f);

	UIButton* runnerButton = new UIButton("RunnerButton", runnerButtonBounds, "Runner", m_font);
	runnerButton->SetButtonBackgroundColor(Rgba8::SAPPHIRE);
	runnerButton->SetButtonHoverColor(Rgba8(50, 80, 150, 120));
	runnerButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		InitializeRunner();
		EnterState(GameState::LEVEL_SELECT);
	});
	g_theUISystem->AddElement(runnerButton);

	UIButton* skaterButton = new UIButton("SkaterButton", skaterButtonBounds, "Skater", m_font);
	skaterButton->SetButtonBackgroundColor(Rgba8::SAPPHIRE);
	skaterButton->SetButtonHoverColor(Rgba8(50, 80, 150, 120));
	skaterButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		InitializeSkater();
		EnterState(GameState::LEVEL_SELECT);
	});
	g_theUISystem->AddElement(skaterButton);

	UIButton* backButton = new UIButton("BackButton", m_controlsButtonBounds, "Back", m_font);
	backButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	backButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	backButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::MAIN_MENU);
	});
	g_theUISystem->AddElement(backButton);

	UIBorder* backBorder = new UIBorder("BackButtonBorder", m_controlsButtonBounds, Rgba8::BLACK, 5.5f);
	g_theUISystem->AddElement(backBorder);
}

void Game::SetupCredits()
{
	UIButton* backButton = new UIButton("BackButton", m_controlsButtonBounds, "Back", m_font);
	backButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	backButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	backButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::MAIN_MENU);
	});
	g_theUISystem->AddElement(backButton);

	UIBorder* backBorder = new UIBorder("BackButtonBorder", m_controlsButtonBounds, Rgba8::BLACK, 5.5f);
	g_theUISystem->AddElement(backBorder);
}

void Game::CreateLevelButton(std::string const& buttonName, int levelIndex, AABB2 buttonBounds)
{
	UIButton* levelButton = new UIButton(buttonName, buttonBounds, Stringf("%d", levelIndex + 1), m_font);
	levelButton->SetButtonBackgroundColor(Rgba8::SAPPHIRE);
	levelButton->SetButtonHoverColor(Rgba8(50, 80, 150, 120));

	if (m_isUnlockMode || m_levelsUnlocked[levelIndex])
	{
		levelButton->SetOnClickCallback([this, levelIndex]()
		{
			g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
			m_currentLevelIndex = levelIndex;
			m_currentLevel = m_levels[levelIndex];
			EnterState(GameState::LEVEL_PLAYING);
		});
	}
	else
	{
		levelButton->SetButtonBackgroundColor(Rgba8::DARKGRAY);
		levelButton->SetButtonHoverColor(Rgba8::DARKGRAY);
	}

	g_theUISystem->AddElement(levelButton);
}

void Game::ToggleDebugText()
{
	m_isDebugTextOn = !m_isDebugTextOn;
}

void Game::Update()
{
	double deltaSeconds = m_gameClock->GetDeltaSeconds();

	if (m_isDebugTextOn)
	{
		std::string timeScaleText = Stringf("[Game Clock] Time: %0.2f, FPS: %0.2f, TimeScale: %0.2f",
			m_gameClock->GetTotalSeconds(), m_gameClock->GetFrameRate(), m_gameClock->GetTimeScale());
		DebugAddScreenText(timeScaleText, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 15.f, Vec2(0.98f, 0.97f), 0.f);
	}

	UpdateUIPresses(static_cast<float>(deltaSeconds));

	if (m_player != nullptr)
	{
		m_player->Update(static_cast<float>(deltaSeconds));
		m_currentLevel->Update(static_cast<float>(deltaSeconds));
	}

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
	UpdateCameras(static_cast<float>(deltaSeconds));
}

void Game::LoadNextLevel()
{
	if (m_isUnlockMode)
	{
		for (int levelIndex = 0; levelIndex < static_cast<int>(m_levelsUnlocked.size()); ++levelIndex)
		{
			m_levelsUnlocked[levelIndex] = true;
		}
	}
	else
	{
		if (m_currentLevelIndex < static_cast<int>(m_levels.size()))
		{
			m_levelsUnlocked[m_currentLevelIndex] = true;
		}
		else
		{
			m_currentLevelIndex = 0;
			EnterState(GameState::MAIN_MENU);
		}
	}

	m_currentLevel = m_levels[m_currentLevelIndex];

	if (m_player)
	{
		m_player->Respawn();
	}
}

void Game::ToggleUnlockMode()
{
	m_isUnlockMode = !m_isUnlockMode;
}

void Game::Render() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	DrawBackgroundTexture();
	g_theUISystem->Render();
	if (m_currentGameState == GameState::MAIN_MENU)
	{
		RenderMainMenu();
	}
	else if (m_currentGameState == GameState::LEVEL_SELECT)
	{
		RenderLevelSelect();
	}
	else if (m_currentGameState == GameState::CHARACTER_SELECT)
	{
		RenderCharacterSelect();
	}
	else if (m_currentGameState == GameState::CONTROLS)
	{
		RenderControls();
	}
	else if (m_currentGameState == GameState::CREDITS)
	{
		RenderCredits();
	}
	else if (m_currentGameState == GameState::GAME_COMPLETE)
	{
		RenderGameComplete();
	}
	g_theRenderer->EndCamera(m_screenCamera);
	if (m_currentGameState == GameState::LEVEL_PLAYING)
	{
		g_theRenderer->BeginCamera(m_gameWorldCamera);
		m_currentLevel->Render();
		m_player->Render();
		g_theRenderer->EndCamera(m_gameWorldCamera);
		DebugRenderWorld(m_gameWorldCamera);
		DebugRenderScreen(m_screenCamera);
	}
}

void Game::Shutdown()
{
	delete m_gameClock;
	m_gameClock = nullptr;

	DestroyPlayer();
	DestroyLevel();

	PlayerDefinition::ClearPlayerDefinitions();
	LevelDefinition::ClearLevelDefinitions();
}

void Game::DestroyPlayer()
{
	delete m_player;
	m_player = nullptr;
}

void Game::DestroyLevel()
{
	for (int levelIndex = 0; levelIndex < static_cast<int>(m_levels.size()); ++levelIndex)
	{
		delete m_levels[levelIndex];
		m_levels[levelIndex] = nullptr;
	}
}

void Game::DrawBackgroundTexture() const
{
	std::vector<Vertex_PCU> bgVerts;
	AddVertsForAABB2D(bgVerts, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), Rgba8::WHITE);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->BindTexture(m_backgroundTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(bgVerts);
}

void Game::KeyInputPresses()
{
	if (m_currentGameState == GameState::MAIN_MENU)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theEventSystem->FireEvent("Quit");
		}
	}
	else if (m_currentGameState == GameState::LEVEL_SELECT)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
			EnterState(GameState::MAIN_MENU);
		}
		if (g_theInput->WasKeyJustPressed('K'))
		{
			ToggleUnlockMode();
		}
	}
	else if (m_currentGameState == GameState::LEVEL_PLAYING)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			EnterState(GameState::MAIN_MENU);
		}
		if (g_theInput->WasKeyJustPressed('R'))
		{
			m_player->Respawn();
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
		{
			ToggleDebugText();
		}
	}
	else if (m_currentGameState == GameState::GAME_COMPLETE)
	{
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			EnterState(GameState::MAIN_MENU);
		}
	}
}

void Game::AdjustForPauseAndTimeDistortion(float deltaSeconds) 
{
	UNUSED(deltaSeconds);

	if (g_theInput->IsKeyDown('T'))
	{
		m_gameClock->SetTimeScale(0.1);
	}
	else
	{
		m_gameClock->SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_gameClock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_gameClock->StepSingleFrame();
	}
}

void Game::HandleCameraInput()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4) && m_currentCameraState == CameraState::FREEFLY)
	{
		m_currentCameraState = CameraState::PLAYER_FOLLOW;
	}
	else if (g_theInput->WasKeyJustPressed(KEYCODE_F4) && m_currentCameraState == CameraState::PLAYER_FOLLOW)
	{
		m_currentCameraState = CameraState::FREEFLY;
	}
}

void Game::UpdateUIPresses(float deltaSeconds)
{
	Vec2 clientPos = g_theInput->GetCursorClientPosition();
	Vec2 worldPos = m_screenCamera.GetClientToWorld(clientPos, g_theWindow->GetClientDimensions());
	g_theUISystem->Update(deltaSeconds, worldPos);

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		g_theUISystem->HandleMouseClick(worldPos);
	}
}

GameState Game::GetCurrentGameState() const
{
	return m_currentGameState;
}

void Game::EnterState(GameState state)
{
	ExitState(m_currentGameState);
	m_currentGameState = state;

	switch (state)
	{
		case GameState::MAIN_MENU:
		{
			SetupUIMainMenu();
			break;
		}
		case GameState::CONTROLS:
		{
			SetupUIControls();
			break;
		}
		case GameState::CREDITS:
		{
			SetupCredits();
			break;
		}
		case GameState::CHARACTER_SELECT:
		{
			SetupUICharacterSelect();
			break;
		}
		case GameState::LEVEL_SELECT:
		{
			SetupUILevelSelect();
			break;
		}
		case GameState::LEVEL_PLAYING:
		{
			m_gameMusicPlayback = g_theAudio->StartSound(m_gameMusic, true, m_musicVolume);
			m_player->Respawn();
			break;
		}
		case GameState::GAME_COMPLETE:
		{
			break;
		}
	}
}

void Game::ExitState(GameState state)
{
	// Clear UI for every state
	g_theUISystem->Clear();

	switch (state)
	{
		case GameState::MAIN_MENU:
		{
			break;
		}
		case GameState::CONTROLS:
		{
			break;
		}
		case GameState::CREDITS:
		{
			break;
		}
		case GameState::CHARACTER_SELECT:
		{
			break;
		}
		case GameState::LEVEL_SELECT:
		{
			break;
		}
		case GameState::LEVEL_PLAYING:
		{
			g_theAudio->StopSound(m_gameMusicPlayback);
			DestroyPlayer();
			break;
		}
		case GameState::GAME_COMPLETE:
		{
			break;
		}
	}
}

void Game::UpdateCameras(float deltaSeconds)
{
	// Screen Camera
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	Mat44 cameraToRender(Vec3::ZAXE, -Vec3::XAXE, Vec3::YAXE, Vec3::ZERO);
	m_gameWorldCamera.SetCameraToRenderTransform(cameraToRender);

	HandleCameraInput();

	if (m_currentCameraState == CameraState::FREEFLY)
	{
		FreeFlyControls(deltaSeconds);
		m_cameraOrientation.m_pitchDegrees = GetClamped(m_cameraOrientation.m_pitchDegrees, -85.f, 85.f);
		m_cameraOrientation.m_rollDegrees = GetClamped(m_cameraOrientation.m_rollDegrees, -45.f, 45.f);
		m_gameWorldCamera.SetPositionAndOrientation(m_cameraPosition, m_cameraOrientation);
		m_gameWorldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 1000.f);
	}

	if (m_currentCameraState == CameraState::PLAYER_FOLLOW)
	{
		if (m_player != nullptr)
		{
			Vec3& playerPos = m_player->m_position;
			m_cameraPosition = playerPos - Vec3(10.f, 0.f, -0.75f);
			m_cameraOrientation = m_player->m_orientation;
			m_gameWorldCamera.SetPositionAndOrientation(m_cameraPosition, m_cameraOrientation);
			m_gameWorldCamera.SetPerspectiveView(2.f, 60.f, 0.1f, 1000.f);
		}
	}
}

void Game::FreeFlyControls(float deltaSeconds)
{
	// Yaw and Pitch with mouse
	m_cameraOrientation.m_yawDegrees += 0.08f * g_theInput->GetCursorClientDelta().x;
	m_cameraOrientation.m_pitchDegrees -= 0.08f * g_theInput->GetCursorClientDelta().y;

	float movementSpeed = 2.f;
	// Increase speed by a factor of 10
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		movementSpeed *= 10.f;
	}

	// Move left or right
	if (g_theInput->IsKeyDown('A'))
	{
		m_cameraPosition += movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		m_cameraPosition += -movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D() * deltaSeconds;
	}

	// Move Forward and Backward
	if (g_theInput->IsKeyDown('W'))
	{
		m_cameraPosition += movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		m_cameraPosition += -movementSpeed * m_cameraOrientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D() * deltaSeconds;
	}

	// Move Up and Down
	if (g_theInput->IsKeyDown('Z'))
	{
		m_cameraPosition += -movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('C'))
	{
		m_cameraPosition += movementSpeed * Vec3::ZAXE * deltaSeconds;
	}
}

void Game::RenderMainMenu() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Runner", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 70.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::RenderLevelSelect() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Level Select", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 70.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::RenderCharacterSelect() const
{
	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Character Select", AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), 70.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::RenderControls() const
{
	AABB2 screenBox = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Move Left:   [A]", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	m_font->AddVertsForTextInBox2D(textVerts, "Jump:    [SPACE]", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.6f));
	m_font->AddVertsForTextInBox2D(textVerts, "Move Right:  [D]", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.5f));
	m_font->AddVertsForTextInBox2D(textVerts, "Pause:       [P]", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.4f));
	m_font->AddVertsForTextInBox2D(textVerts, "Reset:       [R]", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.3f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::RenderCredits() const
{
	AABB2 screenBox = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "Runner created by: Jacob Wilkin", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	m_font->AddVertsForTextInBox2D(textVerts, "Inspired by Joseph Cloutier's Run series on CoolmathGames", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.6f));
	m_font->AddVertsForTextInBox2D(textVerts, "Audio using FMOD, Run 2 main theme from archive.org", screenBox, 25.f, Rgba8::LIMEGREEN);
	m_font->AddVertsForTextInBox2D(textVerts, "Sprites from BrowserGames.com", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.4f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}

void Game::RenderGameComplete() const
{
	AABB2 screenBox = AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	std::vector<Vertex_PCU> textVerts;
	m_font->AddVertsForTextInBox2D(textVerts, "CONGRATULATIONS!", screenBox, 50.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.7f));
	m_font->AddVertsForTextInBox2D(textVerts, "Press ESC to return to the Main Menu", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.5f));
	m_font->AddVertsForTextInBox2D(textVerts, "Thanks for playing my game!", screenBox, 25.f, Rgba8::LIMEGREEN, 1.f, Vec2(0.5f, 0.3f));
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
}
