#pragma once
#include <stdbool.h>

#define GRID_W  20
#define GRID_H  15

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;
typedef struct { int x, y; } Point;

typedef struct {
    Point snake[GRID_W * GRID_H];
    int length;
    Point food;
    Direction dir;
    Direction next_dir;
    int score;
    bool alive;
    bool paused;
} GameState;

void core_init(GameState* gs);
void core_set_direction(GameState* gs, Direction dir);
void core_toggle_pause(GameState* gs);
void core_step(GameState* gs);
bool core_is_alive(const GameState* gs);
bool core_is_paused(const GameState* gs);
int core_get_score(const GameState* gs);