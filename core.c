#include "core.h"
#include <stdlib.h>
#include <time.h>

static void spawn_food(GameState* gs) {
    bool occupied;
    do {
        gs->food.x = rand() % GRID_W;
        gs->food.y = rand() % GRID_H;
        occupied = false;
        for (int i = 0; i < gs->length; ++i) {
            if (gs->snake[i].x == gs->food.x && gs->snake[i].y == gs->food.y) {
                occupied = true;
                break;
            }
        }
    } while (occupied);
}

void core_init(GameState* gs) {
    srand((unsigned)time(NULL));
    gs->length = 3;
    gs->snake[0] = (Point){GRID_W / 2, GRID_H / 2};
    gs->snake[1] = (Point){GRID_W / 2 - 1, GRID_H / 2};
    gs->snake[2] = (Point){GRID_W / 2 - 2, GRID_H / 2};
    gs->dir = DIR_RIGHT;
    gs->next_dir = DIR_RIGHT;
    gs->score = 0;
    gs->alive = true;
    gs->paused = false;
    spawn_food(gs);
}

void core_set_direction(GameState* gs, Direction dir) {
    if (!gs->alive || gs->paused) return;
    if ((gs->dir == DIR_UP && dir == DIR_DOWN) ||
        (gs->dir == DIR_DOWN && dir == DIR_UP) ||
        (gs->dir == DIR_LEFT && dir == DIR_RIGHT) ||
        (gs->dir == DIR_RIGHT && dir == DIR_LEFT)) {
        return;
    }
    gs->next_dir = dir;
}

void core_toggle_pause(GameState* gs) {
    if (!gs->alive) return;
    gs->paused = !gs->paused;
}

void core_step(GameState* gs) {
    if (gs->paused || !gs->alive) return;

    gs->dir = gs->next_dir;
    Point head = gs->snake[0];
    switch (gs->dir) {
        case DIR_UP:    head.y--; break;
        case DIR_DOWN:  head.y++; break;
        case DIR_LEFT:  head.x--; break;
        case DIR_RIGHT: head.x++; break;
    }
    if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
        gs->alive = false; return;
    }
    for (int i = 0; i < gs->length; ++i) {
        if (gs->snake[i].x == head.x && gs->snake[i].y == head.y) {
            gs->alive = false; return;
        }
    }
    bool ate = (head.x == gs->food.x && head.y == gs->food.y);
    if (ate) {
        gs->length++;
        gs->score += 1;
        spawn_food(gs);
    }
    for (int i = gs->length - 1; i > 0; --i) {
        gs->snake[i] = gs->snake[i - 1];
    }
    gs->snake[0] = head;
}

bool core_is_alive(const GameState* gs) { return gs->alive; }
bool core_is_paused(const GameState* gs) { return gs->paused; }
int core_get_score(const GameState* gs) { return gs->score; }