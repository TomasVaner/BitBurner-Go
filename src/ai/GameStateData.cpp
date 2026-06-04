#include "GameStateData.h"

#include "../game/BoardAnalytics.h"

namespace {

void setPlane(std::vector<float>& data, const GameStatePlane plane, const unsigned int index, const float value) {
	data[planeIndex(plane) * AREA + index] = value;
}

float getPlane(const std::vector<float>& data, const GameStatePlane plane, const unsigned int index) {
	return data[planeIndex(plane) * AREA + index];
}

void encodePreviousBoard(std::vector<float>& data, const std::string& prevBoard, const char myColor, const char enemyColor) {
	for (unsigned int index = 0; index < AREA; index++) {
		const char cell = prevBoard[index];
		if (cell == myColor) {
			setPlane(data, GameStatePlane::PrevMyStones, index, 1.0f);
		} else if (cell == enemyColor) {
			setPlane(data, GameStatePlane::PrevEnemyStones, index, 1.0f);
		}
	}
}

void fillPreviousBoardUnavailable(std::vector<float>& data) {
	for (unsigned int index = 0; index < AREA; index++) {
		setPlane(data, GameStatePlane::PrevMyStones, index, PREV_BOARD_UNAVAILABLE);
		setPlane(data, GameStatePlane::PrevEnemyStones, index, PREV_BOARD_UNAVAILABLE);
	}
}

} // namespace

std::vector<float> toVector(GameState* gameState) {
	std::vector<float> data(GAME_STATE_DATA_LENGTH, 0.0f);
	const char myColor = gameState->getColor();
	const char enemyColor = GameState::flipColor(myColor);
	const std::string& board = *gameState->getBoard();
	const ControlledEmpty controlled = computeControlledEmpty(board, myColor);
	const LibertyUrgency liberties = computeLibertyUrgency(board, myColor);

	for (unsigned int index = 0; index < AREA; index++) {
		const char cell = board[index];
		if (cell == '#') {
			setPlane(data, GameStatePlane::Walls, index, 1.0f);
		} else if (cell == myColor) {
			setPlane(data, GameStatePlane::MyStones, index, 1.0f);
		} else if (cell == enemyColor) {
			setPlane(data, GameStatePlane::EnemyStones, index, 1.0f);
		}

		setPlane(data, GameStatePlane::MyControlledEmpty, index, controlled.myControlled[index]);
		setPlane(data, GameStatePlane::EnemyControlledEmpty, index, controlled.enemyControlled[index]);
		setPlane(data, GameStatePlane::MyLibertyUrgency, index, liberties.myUrgency[index]);
		setPlane(data, GameStatePlane::EnemyLibertyUrgency, index, liberties.enemyUrgency[index]);
	}

	for (const int move : *gameState->getValidMoves()) {
		if (move >= 0) {
			setPlane(data, GameStatePlane::LegalMoves, static_cast<unsigned int>(move), 1.0f);
		}
	}

	const std::vector<std::string>* previousBoards = gameState->getPreviousBoards();
	if (previousBoards->empty()) {
		fillPreviousBoardUnavailable(data);
	} else {
		encodePreviousBoard(data, previousBoards->back(), myColor, enemyColor);
	}

	return data;
}

GameState* getGameState(const std::vector<float>& data) {
	std::string board(AREA, '.');
	for (unsigned int index = 0; index < AREA; index++) {
		if (getPlane(data, GameStatePlane::Walls, index) >= 0.5f) {
			board[index] = '#';
		} else if (getPlane(data, GameStatePlane::MyStones, index) >= 0.5f) {
			board[index] = 'X';
		} else if (getPlane(data, GameStatePlane::EnemyStones, index) >= 0.5f) {
			board[index] = 'O';
		}
	}

	return GameState::newGame('X', board);
}
