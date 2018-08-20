#ifndef GAME_ENGINE_H__
#define GAME_ENGINE_H__

#include <random>

#include "Engine.h"
#include "GameObjects.h"

/// State of the game at any given time.
enum class GAMESTATE : int
{
  WELCOME,
  WELCOME_3,
  WELCOME_2,
  WELCOME_1,
  GO,
  PLAY,
  GAMEOVER,
  TRYAGAIN
};

/// Flag used when scene is resetted.
enum class RESET : int
{
  BUT_NOT_THE_PLAYER,
  ALSO_PLAYER_POSITION,
};

/// The travel direction of all enemies.
enum class ENEMY_DIRECTION : int
{
  LEFT,
  RIGHT
};

/// Bounding box with absolute integer values.
struct BoundingBox
{
  float left{0};
  float top{0};
  float right{0};
  float bottom{0};

  /// Checks if a game object position is in a bounding box.
  /// @param o Game object to check.
  bool intersectsWith(const GameObject& o) const;

  /// Move the bounding box towards a given position.
  /// @param pos The position to move the bounding box. Can be negative or
  /// positive.
  void moveBy(const Position& pos);
};

/// High score object that handles points. It can read and write the highscore
/// from and to disk.
class Highscore
{
public:
  /// Adds a point to the score list.
  void addScore();

  /// Finishs a round. Does not write the score to disk, must be called
  /// explicitly.
  void finishScore();

  /// Gets the absolute highscore of all games.
  int getHighscore() const;

  /// Gets the score of the current game.
  int getCurrentScore() const;

  /// Writes the highest score (depending if the current, or previous highscore
  /// is higher) to disk. The file the score is written to is
  /// "spaceinvaders.hscore".
  void writeToDisk() const;

  /// Reads the highscore from the file "spaceinvaders.hscore" if it exists.
  void readFromDisk();

public:
  const int _fileversion{1};
  int _currentScore{0};
  int _oldHighscore{0};
};

class GameEngine : private Engine
{
public:
	using Engine::getStopwatchElapsedSeconds;
  using Engine::startFrame;

  GameEngine();
  ~GameEngine();

  /// Function to handle events, meant to be used in game loop only.
  void handleEvents();

  /// Function to update the game scene. Meant to be used in game loop only.
  void update();

  /// Function to draw the scene to the canvas. Meant to be used in game loop
  /// only.
  void draw();

private:
  /// Resets the game. Is used for instance on startup, or to restart the game
  /// after game is lost.
  /// @param reset		Used to reset entire scene. Player position can be
  /// excluded with corresponding flag
  void resetGame(RESET reset = RESET::ALSO_PLAYER_POSITION);

  /// Initializes the players position. Does not set the health.
  void initPlayer();
  /// Sets the position and health of all enemies.
  void initEnemies();
  /// Resets all rocket objects and sets them to non-alive.
  void initRockets();
  /// Resets all bombs and sets them to non-alive.
  void initBombs();

  /// Draws the hud. Is used inside the ::Draw function during game loop.
  void drawHud();
  /// Draws the player object. Is used inside the ::Draw function during game
  /// loop.
  void drawPlayer();
  /// Draws all enemies. Is used inside the ::Draw function during game loop.
  void drawEnemies();
  /// Draws all rockets. Is used inside the ::Draw function during game loop.
  void drawRockets();
  /// Draws all bombs. Is used inside the ::Draw function during game loop.
  void drawBombs();

  /// Takes actions on enemies. Resets enemies if they left the canvas
  /// (according to the bounding box). Also removes health from player if  the
  /// player if an enemy hit the player. Also destroys an enemy, if it got hit
  /// by a rocket.
  void updateEnemies();
  /// Takes actions on rockets. Sends them in travel direction. Also destroys
  /// them if they left the canvas.
  void updateRockets();
  /// Takes actions on bombs. Destroys them if they left the canvas. Subtracts
  /// health point from player if hit. Also sends bombs from enemies in a
  /// n-interval towards y axis.
  void updateBombs();

  /// Game state, level info and travel direction of enemies
  GAMESTATE _gamestate{GAMESTATE::WELCOME};
  ENEMY_DIRECTION _enemy_direction{ENEMY_DIRECTION::RIGHT};
  Highscore _hscore;
  int _level{1};

  /// Timestamps and fps information
  double _startTimestamp{0.0};
  double _previousTimestamp{0.0};
  double _currentTimestamp{0.0};
  double _timestampOfLastShot{0.0};
  double _timestampOfLastBomb{0.0};
  double _timestampOfLastFpsCalc{0.0};
  double _timestampOfLastFireKey{0.0};
  double _timestampOfGameOver{0.0};
  double _msPerFrame{0.0};
  int _framesCount{0};
  int _fps{60};

  /// Is true if the user lifted the
  /// fire button/key in the last frame
  bool _liftedFireKeyBefore{true};

  /// Scene objects + bounding box of enemies
  Player _player;
  RocketArray _rockets;
  BombArray _bombs;
  EnemyArray _enemies;

  /// Bounding box encloses only aliens that are alive.
  BoundingBox _enemyBbox;

  /// Original bounding box used to determine where rocket hits which column.
  /// Does enclose all aliens, no matter if destroyed or not
  BoundingBox _enemyBboxOriginal;

  /// Random generator for index of enemies dropping bombs
  std::default_random_engine _rd;
  std::uniform_int_distribution<int> _dis;
};

#endif // GAME_ENGINE_H__
