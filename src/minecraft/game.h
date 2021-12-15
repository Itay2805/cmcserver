#pragma once

#include <minecraft/tick_arena.h>

#include <lib/except.h>

extern tick_arena_t* g_current_tick_arena;

err_t start_game_loop();
