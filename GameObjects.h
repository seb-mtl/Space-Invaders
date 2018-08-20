#ifndef GAMEOBJECTS_H__
#define GAMEOBJECTS_H__

#include <array>
#include <cmath>

#include "Config.h"
#include "Engine.h"
#include "GameObjects.h"

struct Position
{
  float _x{0};
  float _y{0};
};

class GameObject
{
public:
  constexpr void setPosition(Position pos) { _pos = pos; }

  Position getPosition() const { return _pos; }

  void setPositionX(float posX) { _pos._x = posX; }

  void setPositionY(float posY) { _pos._y = posY; }

  void destroy() { setHealth(0); }

  constexpr void setHealth(int health) { _health = health; }

  void hit() { --_health; }

  bool isAlive() const { return _health > 0; }

  int getHealth() const { return _health; }

  bool intersectsWith(const GameObject &o,
                      float radius = Engine::SpriteSize / 2) const
  {
    float dx = (_pos._x - o._pos._x);
    float dy = (_pos._y - o._pos._y);
    return sqrt(dx * dx + dy * dy) <= radius;
  }

protected:
  Position _pos;
  int _health{0};
};

class Enemy : public GameObject
{
};

using EnemyArray = std::array<Enemy, Config::ENEMY_COUNT>;

class Bomb : public GameObject
{
};

using BombArray = std::array<Bomb, Config::MAX_BOMB_COUNT>;

class Rocket : public GameObject
{
};

using RocketArray = std::array<Rocket, Config::MAX_ROCKET_COUNT>;

class Player : public GameObject
{
};

#endif // GAMEOBJECTS_H__
