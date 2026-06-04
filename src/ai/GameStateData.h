#ifndef GAME_STATE_DATA_H
#define GAME_STATE_DATA_H

#include "../game/GameState.h"
#include "../game/GameStateConstants.h"

/**
 * Neural input planes (channel index = plane). Order is checkpoint-critical.
 * 0 walls (#)
 * 1 myStones
 * 2 enemyStones
 * 3 legalMoves (valid intersections only, not pass)
 * 4 myControlledEmpty
 * 5 enemyControlledEmpty
 * 6 myLibertyUrgency (1/n on my stones)
 * 7 enemyLibertyUrgency (1/n on enemy stones)
 * 8 lastOpponentMove
 */
constexpr int GAME_STATE_PLANE_COUNT = 9;
constexpr int GAME_STATE_DATA_SIZE[3] = {GAME_STATE_PLANE_COUNT, SIDE_LENGTH, SIDE_LENGTH};
constexpr int GAME_STATE_DATA_LENGTH = GAME_STATE_PLANE_COUNT * SIDE_LENGTH * SIDE_LENGTH;

std::vector<float> toVector(GameState* gameState);

GameState* getGameState(const std::vector<float>& data);

#endif
