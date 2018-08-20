#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

#include "Config.h"
#include "GameEngine.h"

#if __cplusplus > \
    201703L // thats my general way to ensure todos don't get ignored for long
#define CPP20
#endif

#if defined(__GNUC__) || defined(__clang__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

bool BoundingBox::intersectsWith(const GameObject &o) const
{
  Position pos = o.getPosition();
  return (pos._x >= left && pos._x <= right && pos._y >= top &&
          pos._y <= bottom);
}

void BoundingBox::moveBy(const Position &pos)
{
  left += pos._x;
  right += pos._x;
  top += pos._y;
  bottom += pos._y;
}

template <typename T, size_t SIZE>
static BoundingBox getBoundingBoxOf(const std::array<T, SIZE> &arr)
{
  // use static_assert until C++20 concepts are available
  static_assert(std::is_same<GameObject, T>::value ||
                    std::is_base_of<GameObject, T>::value,
                "only supports arrays of type GameObject");
#if defined(CPP20)
  // see comment above
  static_assert(false);
#endif

  bool atLeastOneEnemyAlive{false};

  float left{std::numeric_limits<float>::max()};
  float top{std::numeric_limits<float>::max()};
  float right{std::numeric_limits<float>::min()};
  float bottom{std::numeric_limits<float>::min()};
  for (const auto &o : arr)
  {
    if (!o.isAlive())
      continue;

    atLeastOneEnemyAlive = true;

    Position pos = o.getPosition();
    if (pos._x < left)
      left = pos._x;
    if (pos._y < top)
      top = pos._y;
    if (pos._x > right)
      right = pos._x;
    if (pos._y > bottom)
      bottom = pos._y;
  }

  if (atLeastOneEnemyAlive)
  {
    // The origin position of a sprite is the middle, therefore the
    // bounding box is expanded in all directions by half the sprite size
    left -= Engine::SpriteSize / 2;
    top -= Engine::SpriteSize / 2;
    right += Engine::SpriteSize / 2;
    bottom += Engine::SpriteSize / 2;

    return {left, top, right, bottom};
  }
  else
  {
    return {};
  }
}

void Highscore::finishScore()
{
  if (_currentScore > _oldHighscore)
    _oldHighscore = _currentScore;
  _currentScore = 0;
}

void Highscore::addScore() { _currentScore++; }

int Highscore::getCurrentScore() const { return _currentScore; }

int Highscore::getHighscore() const { return _oldHighscore; }

void Highscore::writeToDisk() const
{
  std::ofstream hscore{"spaceinvaders.hscore"};
  // P.S. I tend to avoid using reinterpret_cast if possible due to its
  // tendency to be very restrictive but since char* and byte casts are well
  // defined this is safe
  hscore.write(reinterpret_cast<const char *>(&_fileversion),
               sizeof(_fileversion));
  if (_currentScore > _oldHighscore)
    hscore.write(reinterpret_cast<const char *>(&_currentScore),
                 sizeof(_currentScore));
  else
    hscore.write(reinterpret_cast<const char *>(&_oldHighscore),
                 sizeof(_oldHighscore));
}

void Highscore::readFromDisk()
{
  std::ifstream hscore{"spaceinvaders.hscore"};

  int fileversion{0};
  hscore.read(reinterpret_cast<char *>(&fileversion), sizeof(fileversion));
  if (fileversion == 1)
  {
    hscore.read(reinterpret_cast<char *>(&_oldHighscore),
                sizeof(_oldHighscore));
  }
}

GameEngine::GameEngine()
{
  _dis = std::uniform_int_distribution<int>{0, Config::ENEMY_COUNT - 1};
  _startTimestamp = getStopwatchElapsedSeconds();
  _hscore.readFromDisk();
  _level = 1;

  resetGame();
}

GameEngine::~GameEngine() { _hscore.writeToDisk(); }

void GameEngine::resetGame(RESET reset)
{
  if (reset == RESET::ALSO_PLAYER_POSITION)
  {
    initPlayer();
  }

  _player.setHealth(Config::PLAYER_HEALTH);

  initEnemies();
  initRockets();
  initBombs();
}

void GameEngine::initPlayer()
{
  float posX = Engine::CanvasWidth / 2;
  float posY = Engine::CanvasHeight - (Engine::SpriteSize / 2);
  _player.setPosition({posX, posY});
}

void GameEngine::initEnemies()
{
  // the origin of the enemies is right below the top bar
  int startY{Engine::FontRowHeight + 10};

  int i{0};
  for (auto &e : _enemies)
  {
    float col = (i / Config::ENEMY_ROWS);
    float row = (i % Config::ENEMY_ROWS);
    e.setPosition(
        {col * Engine::SpriteSize + (Engine::SpriteSize / 2),
         startY + row * Engine::SpriteSize + +(Engine::SpriteSize / 2)});
    e.setHealth(1);
    ++i;
  }

  _enemyBbox = getBoundingBoxOf(_enemies);
  _enemyBboxOriginal = _enemyBbox;
}

void GameEngine::initRockets()
{
  for (auto &r : _rockets)
  {
    r.setHealth(0);
  }
}

void GameEngine::initBombs()
{
  for (auto &b : _bombs)
  {
    b.setHealth(0);
  }
}

void GameEngine::draw()
{
  drawPlayer();
  drawEnemies();
  drawRockets();
  drawBombs();
  drawHud();
}

void GameEngine::drawHud()
{
  // draw health level
  for (int i = 0; i < _player.getHealth(); ++i)
  {
    drawSprite(Engine::Sprite::Player, (i * Engine::SpriteSize), 5);
  }

  // draw current score
  std::string cscore =
      "Current Score: " + std::to_string(_hscore.getCurrentScore());
  drawText(cscore.c_str(),
           (Engine::CanvasWidth -
            static_cast<int>(cscore.size()) * Engine::FontWidth) /
               2,
           Engine::SpriteSize - Engine::FontRowHeight);

  // draw highscore
  std::string hscore = "Highscore: " + std::to_string(_hscore.getHighscore());
  drawText(hscore.c_str(),
           Engine::CanvasWidth -
               static_cast<int>(hscore.size()) * Engine::FontWidth,
           Engine::SpriteSize - Engine::FontRowHeight);

  // draw fps
  if (_currentTimestamp > _timestampOfLastFpsCalc + 1.0)
  {
    _fps = _framesCount;
    _framesCount = 0;
    _timestampOfLastFpsCalc = _currentTimestamp;
  }
  _framesCount++;
  std::string sfps = std::to_string(_fps) + "FPS";
  drawText(sfps.c_str(), 0, Engine::CanvasHeight - Engine::FontRowHeight);

  // draw centered message
  const char *message{nullptr};
  const char *message2{nullptr};
  switch (_gamestate)
  {
  case GAMESTATE::PLAY:
    return;
  case GAMESTATE::GAMEOVER:
    message = "Game Over :-(";
    // display the message 2 seconds after game over
    if (_currentTimestamp - _timestampOfGameOver > 2.0)
      message2 = "Press space to try again";
    break;
  case GAMESTATE::WELCOME:
    message = "Welcome";
    break;
  case GAMESTATE::WELCOME_3:
    message = "3";
    break;
  case GAMESTATE::WELCOME_2:
    message = "2";
    break;
  case GAMESTATE::WELCOME_1:
    message = "1";
    break;
  case GAMESTATE::GO:
  default:
    message = "Go!";
    break;
  }

  if (message)
  {
    drawText(message,
             (Engine::CanvasWidth -
              (static_cast<int>(strlen(message)) - 1) * Engine::FontWidth) /
                 2,
             (Engine::CanvasHeight - Engine::FontRowHeight) / 2);
  }

  if (message2)
  {
    drawText(message2,
             (Engine::CanvasWidth -
              (static_cast<int>(strlen(message2)) - 1) * Engine::FontWidth) /
                 2,
             (Engine::CanvasHeight - Engine::FontRowHeight) / 2 +
                 (Engine::FontRowHeight * 2));
  }
}

void GameEngine::drawPlayer()
{
  Position pos = _player.getPosition();
  drawSprite(Engine::Sprite::Player, int(pos._x - Engine::SpriteSize / 2),
             int(pos._y - Engine::SpriteSize / 2));
}

void GameEngine::drawEnemies()
{
  bool altSprite = false;
  for (const auto &e : _enemies)
  {
    if (e.isAlive())
    {
      Position pos = e.getPosition();
      drawSprite(altSprite ? Engine::Sprite::Enemy1 : Engine::Sprite::Enemy2,
                 int(pos._x - Engine::SpriteSize / 2),
                 int(pos._y - Engine::SpriteSize / 2));
    }
    altSprite = altSprite == false;
  }
}

void GameEngine::drawRockets()
{
  for (const auto &r : _rockets)
  {
    if (!r.isAlive())
      continue;

    Position pos = r.getPosition();
    drawSprite(Engine::Sprite::Rocket, int(pos._x - Engine::SpriteSize / 2),
               int(pos._y - Engine::SpriteSize / 2));
  }
}

void GameEngine::drawBombs()
{
  for (const auto &b : _bombs)
  {
    if (!b.isAlive())
      continue;

    Position pos = b.getPosition();
    drawSprite(Engine::Sprite::Bomb, int(pos._x - Engine::SpriteSize / 2),
               int(pos._y - Engine::SpriteSize / 2));
  }
}

void GameEngine::handleEvents()
{
  // update timer variables
  _currentTimestamp = getStopwatchElapsedSeconds();
  _msPerFrame = _currentTimestamp - _previousTimestamp;
  _previousTimestamp = _currentTimestamp;

  Engine::PlayerInput keys = Engine::getPlayerInput();

  if (keys.fire)
  {
    // The fire key is only recognized as an input if the user
    // didn't press it the frame before. Otherwise the spaceship
    // would automatically shoot. And also the "Game Over" dialog would
    // disappear after 1 frame if the space bar is constantly pressed
    if (_liftedFireKeyBefore)
    {
      if (_gamestate == GAMESTATE::GAMEOVER)
      {
        if (_currentTimestamp - _timestampOfLastFireKey > 2.0)
        {
          resetGame(RESET::BUT_NOT_THE_PLAYER);
          _hscore.finishScore();
          _hscore.writeToDisk();
          _gamestate = GAMESTATE::TRYAGAIN;
        }
        return;
      }
      else if (_gamestate ==
               GAMESTATE::PLAY) // can only shoot while in play state
      {
        if (_currentTimestamp - _timestampOfLastShot >
            Config::TIME_BETWEEN_SHOTS) // restrict shooting per second
        {
          for (auto &r : _rockets)
          {
            if (!r.isAlive())
            {
              _timestampOfLastShot = getStopwatchElapsedSeconds();
              r.setPosition(_player.getPosition());
              r.setHealth(1);
              break;
            }
          }
        }
      }
    }
    _liftedFireKeyBefore = false;
    _timestampOfLastFireKey = _currentTimestamp;
  }
  else
  {
    _liftedFireKeyBefore = true;
  }

  // user can move in any gamestate (e.g. welcome screen
  // or during game mode but not when game is over
  if (_gamestate != GAMESTATE::GAMEOVER)
  {
    if (keys.left)
    {
      Position pos = _player.getPosition();
      if (pos._x > 0)
      {
        _player.setPositionX(pos._x - 400 * _msPerFrame);
      }
    }

    if (keys.right)
    {
      Position pos = _player.getPosition();
      if (pos._x < Engine::CanvasWidth - Engine::SpriteSize)
      {
        _player.setPositionX(pos._x + 400 * _msPerFrame);
      }
    }
  }
}

void GameEngine::update()
{
  int sizeof_enemies = sizeof _enemies;
  int sizeof_rockets = sizeof _rockets;

  float passedSeconds = _currentTimestamp - _startTimestamp;

  if (_gamestate == GAMESTATE::PLAY)
  {
    updateEnemies();
    updateBombs();
    updateRockets();
  }
  else if (_gamestate == GAMESTATE::GAMEOVER)
  {
    // nothing to compute here...
  }
  else if (passedSeconds > 6)
  {
    _gamestate = GAMESTATE::PLAY;
    return;
  }
  else if (passedSeconds > 5)
  {
    _gamestate = GAMESTATE::GO;
  }
  else if (passedSeconds > 4)
  {
    _gamestate = GAMESTATE::WELCOME_1;
  }
  else if (passedSeconds > 3)
  {
    _gamestate = GAMESTATE::WELCOME_2;
  }
  else if (passedSeconds > 2)
  {
    _gamestate = GAMESTATE::WELCOME_3;
  }
  else
  {
    _gamestate = GAMESTATE::WELCOME;
  }
}

void GameEngine::updateRockets()
{
  // delete rockets which are out of
  for (auto &r : _rockets)
  {
    if (r.isAlive())
    {
      Position pos = r.getPosition();

      // make rocket travel on y-axis and
      // destroy rocket until it left canvas
      if (pos._y > -Engine::SpriteSize)
      {
        r.setPositionY(pos._y - 1);
      }
      else
      {
        r.destroy();
      }
    }
  }
}

void GameEngine::updateBombs()
{
  for (auto &b : _bombs)
  {
    if (b.isAlive())
    {
      Position pos = b.getPosition();
      // if aliens are out of screen
      if (pos._y > Engine::CanvasHeight)
      {
        b.destroy();
      }
      else if (b.intersectsWith(_player))
      {
        b.destroy();
        _player.hit();
        if (!_player.isAlive())
        {
          _timestampOfGameOver = _currentTimestamp;
          _gamestate = GAMESTATE::GAMEOVER;
          return;
        }
      }
      else
      {
        b.setPositionY(pos._y + 150 * _msPerFrame);
      }
    }
    else
    {
      // case to drop bombs, which are restricted to drop by time
      if (_currentTimestamp - _timestampOfLastBomb < Config::TIME_BETWEEN_BOMBS)
        continue;

      // iterate through all alive enemies (repeat over at the end)
      // until the random generated index is hit
      int index = _dis(_rd);
      auto it = _enemies.begin();
      do
      {
        if (it == _enemies.end())
          it = _enemies.begin();

        if (it->isAlive())
        {
          if (index == 0)
          {
            _timestampOfLastBomb = _currentTimestamp;
            b.setPosition(it->getPosition());
            b.setHealth(1);
          }
        }
        it++;
      } while (index-- > 0);
    }
  }
}

void GameEngine::updateEnemies()
{
  if (_enemyBbox.bottom >= Engine::CanvasHeight)
  {
    // If an alien reaches the bottom of the screen, the player loses and the
    // game is over.
    _gamestate = GAMESTATE::GAMEOVER;
    _hscore.writeToDisk();
    return;
  }
  else if (_enemyBbox.intersectsWith(_player))
  {
    for (auto &e : _enemies)
    {
      if (e.intersectsWith(_player))
      {
        // If the alien collides with the player, the alien
        // is destroyed and the player's health is decreased.
        e.destroy();
        _player.hit();

        if (!_player.isAlive())
        {
          _gamestate = GAMESTATE::GAMEOVER;
          _hscore.writeToDisk();
          return;
        }
        break;
      }
    }
  }

  // destroy an enemy if it got hit by a rocket
  bool enemyDied{false};
  for (auto &r : _rockets)
  {
    if (!r.isAlive())
      continue;

    // Explanation:
    // There are two bounding boxes, one (_enemyBboxOriginal) that encloses
    // all enemies, and another (_enemyBbox) only those that are alive.
    // _enemyBbox is used to verify if a rocket even hits the region of
    // alive enemies. If that is the case, _enemyBboxOriginal is used to
    // determine the column and an additional radius check is applied on all
    // enemies from bottom to top. That results in a worst case complexity
    // for the enemy lookup of Big-O(Config::ENEMY_ROWS)

    // check if rocket intersects
    // with the bounding box of the enemies
    if (!_enemyBbox.intersectsWith(r))
      continue;
    Position rpos = r.getPosition();

    // Calculate the rocket position in the original bounding box to
    // calculate the column
    int column =
        int(float(rpos._x - _enemyBboxOriginal.left) / Engine::SpriteSize);

    // if outer range
    if (unlikely(column > Config::ENEMY_COLS || column < 0))
      continue;
    else if (unlikely(column ==
                      Config::ENEMY_COLS)) // happens if rocket hits the right
                                           // side of the bbox
      column--;

    // iterate through all enemies of that column from bottom to top
    int end = column * Config::ENEMY_ROWS;
    for (int i = (column + 1) * Config::ENEMY_ROWS - 1; i >= end; --i)
    {
      Enemy &e = _enemies[i];
      if (e.isAlive() && e.intersectsWith(r))
      {
        r.destroy();
        e.destroy();

        enemyDied = true;
        _hscore.addScore();
        break;
      }
    }
  }

  if (enemyDied)
  {
    bool atLeastOneEnemyAlive =
        std::any_of(_enemies.begin(), _enemies.end(),
                    [](const auto &e) { return e.isAlive(); });
    if (!atLeastOneEnemyAlive)
    {
      // if all enemies are destroyed,
      // reset all of them and start over
      initEnemies();
      ++_level;
    }
    else
    {
      // update bounding box if at least one enemy died
      _enemyBbox = getBoundingBoxOf(_enemies);
    }
  }

  // move all enemies in travel direction
  float travelStepX{0};
  float travelStepY{0};
  if (_enemy_direction == ENEMY_DIRECTION::RIGHT)
  {
    if (_enemyBbox.right < Engine::CanvasWidth)
    {
      travelStepX = 200 * _msPerFrame;
    }
    else
    {
      travelStepY = 10 * _level; // down step is dependend on level
      _enemy_direction = ENEMY_DIRECTION::LEFT;
      travelStepX = -1;
    }
  }
  else // _enemy_direction == ENEMY_DIRECTION::RIGHT
  {
    if (_enemyBbox.left > 0)
    {
      travelStepX = -(200 * _msPerFrame);
    }
    else
    {
      travelStepY = 10 * _level; // down step is dependend on level
      _enemy_direction = ENEMY_DIRECTION::RIGHT;
      travelStepX = 1;
    }
  }

  for (auto &e : _enemies)
  {
    e.setPositionX(e.getPosition()._x + travelStepX);
    e.setPositionY(e.getPosition()._y + travelStepY);
  }

  _enemyBbox.moveBy({travelStepX, travelStepY});
  _enemyBboxOriginal.moveBy({travelStepX, travelStepY});
}
