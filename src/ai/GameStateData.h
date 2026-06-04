#ifndef GAME_STATE_DATA_H
#define GAME_STATE_DATA_H

#include "../game/GameState.h"
#include "../game/GameStateConstants.h"

/** Fills PrevMyStones / PrevEnemyStones when there is no prior board (first turn). */
constexpr float PREV_BOARD_UNAVAILABLE = -1.0f;

/**
 * Neural input planes. Underlying order is checkpoint-critical.
 */
enum class GameStatePlane : int {
	Walls,
	MyStones,
	EnemyStones,
	LegalMoves,
	MyControlledEmpty,
	EnemyControlledEmpty,
	MyLibertyUrgency,
	EnemyLibertyUrgency,
	PrevMyStones,
	PrevEnemyStones,
	Count
};

constexpr int planeIndex(const GameStatePlane plane) {
	return static_cast<int>(plane);
}

constexpr int GAME_STATE_PLANE_COUNT = planeIndex(GameStatePlane::Count);
constexpr int GAME_STATE_DATA_SIZE[3] = {GAME_STATE_PLANE_COUNT, SIDE_LENGTH, SIDE_LENGTH};
constexpr int GAME_STATE_DATA_LENGTH = GAME_STATE_PLANE_COUNT * SIDE_LENGTH * SIDE_LENGTH;

std::vector<float> toVector(GameState* gameState);

GameState* getGameState(const std::vector<float>& data);

#endif
