#ifndef CONFIG_H__
#define CONFIG_H__

struct Config
{
  static const int MAX_ROCKET_COUNT{10};
  static const int MAX_BOMB_COUNT{10};
  static const int ENEMY_COUNT{50};
  static const int ENEMY_ROWS{5};
  static const int ENEMY_COLS{ENEMY_COUNT / ENEMY_ROWS};
  static const int PLAYER_HEALTH{3};
  static constexpr double TIME_BETWEEN_SHOTS{0.25};
  static constexpr double TIME_BETWEEN_BOMBS{0.35};

  // assert guarantees
  static_assert(float(ENEMY_COUNT) / ENEMY_ROWS == ENEMY_COUNT / ENEMY_ROWS,
                "ENEMY_COUNT must be dividable by ENEMY_ROWS");
};

#endif // CONFIG_H__
