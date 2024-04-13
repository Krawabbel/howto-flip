#pragma once

#include <gui/view.h>

#define GAME_DATA_PATH (EXT_PATH("chip8"))

typedef struct Chip8Game Game;

Game* game_alloc();

View* game_get_view(Game* game);

void game_start(Game* game, FuriString* path);

void game_free(Game* game);
