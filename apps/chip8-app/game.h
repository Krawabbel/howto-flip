#pragma once

#include <gui/view.h>

typedef struct Chip8Game Game;

Game* game_alloc();

View* game_get_view(Game* game);

void game_start(Game* game, FuriString* path);

void game_free(Game* game);
