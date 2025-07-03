#include "Game/Game.h"
#include "Game/GameCommon.h"
#include "Game/App.h"
#include "Game/PlayerDefinition.hpp"
#include "Game/Player.hpp"

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
	g_theDevConsole->AddLine(Rgba8::LIGHTYELLOW, "F4    - Toggle camera switch");
	g_theDevConsole->AddLine(Rgba8::SEAWEED, "----------------------------------------------------------------------");

	m_backgroundTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/galaxy.jpg");
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	m_phongShader = g_theRenderer->CreateOrGetShader("Data/Shaders/Phong", VertexType::VERTEX_PCUTBN);

	EnterState(GameState::MAIN_MENU);

	PlayerDefinition::InitializePlayerDefintions();
	LayoutLevelOne();

	CreateLevelGeometry();
	CreateBuffers();
}

void Game::CreateLevelGeometry()
{
	// Blocks
	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block const* block = m_blocks[blockIndex];
		AddVertsForAABB3D(m_blockTBNVerts, m_blockIndices, block->m_bounds, block->m_blockColor);
	}

	// End Goal
	AddVertsForSphere3D(m_blockTBNVerts, m_blockIndices, m_endGoal->m_center, m_endGoal->m_radius, m_endGoal->m_endGoalColor);
}

void Game::CreateBuffers()
{
	// Create buffers and copy to GPU
	m_blockVBO = g_theRenderer->CreateVertexBuffer(static_cast<unsigned int>(m_blockTBNVerts.size()) * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_blockIBO = g_theRenderer->CreateIndexBuffer(static_cast<unsigned int>(m_blockIndices.size()) * sizeof(unsigned int), sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_blockTBNVerts.data(), m_blockVBO->GetSize(), m_blockVBO);
	g_theRenderer->CopyCPUToGPU(m_blockIndices.data(), m_blockIBO->GetSize(), m_blockIBO);
}

void Game::LayoutLevelOne()
{
	// Start Block that is a large block for the starting platform
	SpawnBlock(Vec3(0.f, 0.f, -0.5f), Vec3(10.f, 10.f, 0.f), Rgba8::GREEN);

	// Series of platforming blocks
	SpawnBlock(Vec3(10.f, 2.f, 1.f), Vec3(6.f, 6.f, 0.8f), Rgba8::BLUE);
	SpawnBlock(Vec3(20.f, -2.f, 1.f), Vec3(6.f, 6.f, 0.8f), Rgba8::BLUE);  
	SpawnBlock(Vec3(30.f, 0.f, 2.f), Vec3(6.f, 6.f, 0.8f), Rgba8::BLUE);  
	SpawnBlock(Vec3(40.f, 4.f, 3.f), Vec3(6.f, 3.f, 0.8f), Rgba8::BLUE);  
	SpawnBlock(Vec3(50.f, 0.f, 3.f), Vec3(6.f, 3.f, 0.8f), Rgba8::BLUE);  
	SpawnBlock(Vec3(60.f, -3.f, 4.f), Vec3(6.f, 1.f, 0.8f), Rgba8::BLUE); 

	// Final jump block before the goal
	SpawnBlock(Vec3(70.f, 2.f, 5.f), Vec3(4.f, 3.f, 0.8f), Rgba8::BLUE);

	// End Goal
	m_endGoal = new EndGoal(Vec3(75.f, 0.f, 7.f), 1.f, Rgba8::GOLD);
}

void Game::InitializePlayer()
{
	m_player = new Player(this, Vec3::ZERO, EulerAngles::ZERO, Rgba8::SAPPHIRE);
}

void Game::SpawnBlock(Vec3 center, Vec3 dimensions, Rgba8 color)
{
	Vec3 halfDims = dimensions * 0.5f;
	AABB3 bounds(center - halfDims, center + halfDims);
	m_blocks.push_back(new Block{ bounds, color });
}

void Game::SetupUIMainMenu()
{
	UIButton* startButton = new UIButton("StartButton", m_startButtonBounds, "Start", m_font);
	startButton->SetButtonBackgroundColor(Rgba8::SEAWEED);
	startButton->SetButtonHoverColor(Rgba8(20, 60, 20, 120));
	startButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::LEVEL_SELECT);
	});
	g_theUISystem->AddElement(startButton);

	UIButton* exitButton = new UIButton("ExitButton", m_exitButtonBounds, "Exit", m_font);
	exitButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	exitButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	exitButton->SetOnClickCallback([]()
	{
		g_theEventSystem->FireEvent("Quit");
	});
	g_theUISystem->AddElement(exitButton);

	UIButton* controlsButton = new UIButton("ControlsButton", m_controlsButtonBounds, "Controls", m_font);
	controlsButton->SetButtonBackgroundColor(Rgba8::SEAWEED);
	controlsButton->SetButtonHoverColor(Rgba8(20, 60, 20, 120));
	controlsButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::CONTROLS);
	});
	g_theUISystem->AddElement(controlsButton);
}

void Game::SetupUILevelSelect()
{
	UIButton* levelOneButton = new UIButton("LevelOneButton", m_startButtonBounds, "1", m_font);
	levelOneButton->SetButtonBackgroundColor(Rgba8::SAPPHIRE);
	levelOneButton->SetButtonHoverColor(Rgba8(50, 80, 150, 120));
	levelOneButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::LEVEL_PLAYING);
	});
	g_theUISystem->AddElement(levelOneButton);

	UIButton* backButton = new UIButton("BackButton", m_exitButtonBounds, "Back", m_font);
	backButton->SetButtonBackgroundColor(Rgba8::DARKRED);
	backButton->SetButtonHoverColor(Rgba8(139, 0, 0, 120));
	backButton->SetOnClickCallback([this]()
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		EnterState(GameState::MAIN_MENU);
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
}

void Game::Update()
{
	// Setting clock time variables
	double deltaSeconds = m_gameClock.GetDeltaSeconds();
	double totalTime    = Clock::GetSystemClock().GetTotalSeconds();
	double frameRate    = Clock::GetSystemClock().GetFrameRate();
	double scale        = Clock::GetSystemClock().GetTimeScale();

	// Set text for time, FPS, and scale
	std::string timeScaleText = Stringf("Time: %0.2fs FPS: %0.2f Scale: %0.2f", totalTime, frameRate, scale);
	DebugAddScreenText(timeScaleText, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), 15.f, Vec2(0.98f, 0.97f), 0.f);

	UpdateUIPresses(static_cast<float>(deltaSeconds));

	if (m_player != nullptr)
	{
		m_player->Update(static_cast<float>(deltaSeconds));
		CollidePlayerWithBlocks();
		CheckDeathBounds(m_player);
		CheckPlayerVsEndGoal(m_player);
	}

	AdjustForPauseAndTimeDistortion(static_cast<float>(deltaSeconds));
	KeyInputPresses();
	UpdateCameras(static_cast<float>(deltaSeconds));
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
	else if (m_currentGameState == GameState::CONTROLS)
	{
		RenderControls();
	}
	g_theRenderer->EndCamera(m_screenCamera);
	if (m_currentGameState == GameState::LEVEL_PLAYING)
	{
		g_theRenderer->BeginCamera(m_gameWorldCamera);
		DrawLevelItems();
		m_player->Render();
		g_theRenderer->EndCamera(m_gameWorldCamera);
		DebugRenderWorld(m_gameWorldCamera);
		DebugRenderScreen(m_screenCamera);
	}
}

void Game::Shutdown()
{
	DestroyPlayer();
	DestroyGeometry();
	ClearBuffers();
	PlayerDefinition::ClearPlayerDefinitions();
}

void Game::ClearBuffers()
{
	delete m_blockVBO;
	m_blockVBO = nullptr;

	delete m_blockIBO;
	m_blockIBO = nullptr;
}

void Game::DestroyGeometry()
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

void Game::DestroyPlayer()
{
	delete m_player;
	m_player = nullptr;
}

void Game::DrawLevelItems() const
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
		if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
			EnterState(GameState::LEVEL_SELECT);
		}
	}
	else if (m_currentGameState == GameState::LEVEL_SELECT)
	{
		if (g_theInput->WasKeyJustPressed(' '))
		{
			g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
			EnterState(GameState::LEVEL_PLAYING);
		}
		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
			EnterState(GameState::MAIN_MENU);
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
	}
}

void Game::AdjustForPauseAndTimeDistortion(float deltaSeconds) {

	UNUSED(deltaSeconds);

	if (g_theInput->IsKeyDown('T'))
	{
		m_gameClock.SetTimeScale(0.1);
	}
	else
	{
		m_gameClock.SetTimeScale(1.0);
	}

	if (g_theInput->WasKeyJustPressed('P'))
	{
		g_theAudio->StartSound(m_clickSound, false, m_musicVolume);
		m_gameClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_gameClock.StepSingleFrame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) && m_currentGameState == GameState::MAIN_MENU)
	{
		g_theEventSystem->FireEvent("Quit");
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

void Game::CollidePlayerWithBlocks()
{
	if (m_player != nullptr)
	{
		CollidePlayerWithBlocks(m_player);
	}
}

void Game::CollidePlayerWithBlocks(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;
	playerCharacter->m_isGrounded = false;

	for (int blockIndex = 0; blockIndex < static_cast<int>(m_blocks.size()); ++blockIndex)
	{
		Block*& block = m_blocks[blockIndex];
		if (PushZCylinderOutOfFixedAABB3D(playerPos, radius, height, block->m_bounds))
		{
			float playerBottomZ = playerPos.z - (height * 0.5f);
			float blockTopZ = block->m_bounds.m_maxs.z;

			if (fabsf(playerBottomZ - blockTopZ) < 0.05f)
			{
				playerCharacter->m_isGrounded = true;
				playerCharacter->m_velocity.z = 0.f;
			}

			Vec3 blockCenter = block->m_bounds.GetCenter();
			Vec3 pushDirection = (playerPos - blockCenter).GetNormalized();
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

void Game::CheckDeathBounds(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;

	if (DoZCylinderAndAABB3Overlap3D(playerPos, radius, height, m_deathBounds))
	{
		playerCharacter->Respawn();
	}
}

void Game::CheckPlayerVsEndGoal(Player* playerCharacter)
{
	Vec3& playerPos = playerCharacter->m_position;
	float radius = playerCharacter->m_physicsRadius;
	float height = playerCharacter->m_physicsHeight;

	if (DoZCylinderAndSphereOverlap3D(playerPos, radius, height, m_endGoal->m_center, m_endGoal->m_radius))
	{
		// Temporarily going back to main menu after overlap
		g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Congratulations, you have beat Level One!");
		EnterState(GameState::MAIN_MENU);
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
		case GameState::LEVEL_SELECT:
		{
			SetupUILevelSelect();
			break;
		}
		case GameState::LEVEL_PLAYING:
		{
			m_gameMusicPlayback = g_theAudio->StartSound(m_gameMusic, true, m_musicVolume);
			InitializePlayer();
			break;
		}
		case GameState::LEVEL_COMPLETE:
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
		case GameState::LEVEL_COMPLETE:
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
			m_cameraPosition = playerPos - Vec3(10.f, 0.f, -0.5f);
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
