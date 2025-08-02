#pragma once
#include "Engine/Math/RandomNumberGenerator.h"

class App;
class Game;
class Renderer;
class InputSystem;
class AudioSystem;
class UISystem;
class Window;
struct Vec2;
struct Rgba8;
// -----------------------------------------------------------------------------
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;
constexpr float SCREEN_CENTER_X = SCREEN_SIZE_X / 2.f;
constexpr float SCREEN_CENTER_Y = SCREEN_SIZE_Y / 2.f;
// -----------------------------------------------------------------------------
constexpr float GRAVITY_FORCE = -24.0f;
constexpr float MAX_FALL_SPEED = -30.f;
// -----------------------------------------------------------------------------
enum class GameState
{
	NONE,
	MAIN_MENU,
	CONTROLS,
	CREDITS,
	CHARACTER_SELECT,
	LEVEL_SELECT,
	LEVEL_PLAYING,
	GAME_COMPLETE
};
// -----------------------------------------------------------------------------
enum class CameraState
{
	NONE,
	FREEFLY,
	PLAYER_FOLLOW
};
// -----------------------------------------------------------------------------
extern App* g_theApp;
extern Game* g_theGame;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern UISystem* g_theUISystem;
extern Window* g_theWindow;
// -----------------------------------------------------------------------------
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
