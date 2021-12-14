#pragma once

#include <minecraft/protocol/packet_arena.h>

#include <lib/except.h>

extern packet_arena_t* g_current_tick_arena;

err_t start_game_loop();
