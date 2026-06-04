#include "GameStateData.h"

#include "../game/BoardAnalytics.h"

namespace {

void setPlane(std::vector<float>& data, const int plane, const unsigned int index, const float value) {
	data[plane * AREA + index] = value;
}

float getPlane(const std::vector<float>& data, const int plane, const unsigned int index) {
	return data[plane * AREA + index];
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
			setPlane(data, 0, index, 1.0f);
		} else if (cell == myColor) {
			setPlane(data, 1, index, 1.0f);
		} else if (cell == enemyColor) {
			setPlane(data, 2, index, 1.0f);
		}

		setPlane(data, 4, index, controlled.myControlled[index]);
		setPlane(data, 5, index, controlled.enemyControlled[index]);
		setPlane(data, 6, index, liberties.myUrgency[index]);
		setPlane(data, 7, index, liberties.enemyUrgency[index]);
	}

	for (const int move : *gameState->getValidMoves()) {
		if (move >= 0) {
			setPlane(data, 3, static_cast<unsigned int>(move), 1.0f);
		}
	}

	const int lastOpponentMove = gameState->getLastOpponentMoveIndex();
	if (lastOpponentMove >= 0 && static_cast<unsigned int>(lastOpponentMove) < AREA) {
		setPlane(data, 8, static_cast<unsigned int>(lastOpponentMove), 1.0f);
	}

	return data;
}

GameState* getGameState(const std::vector<float>& data) {
	std::string board(AREA, '.');
	for (unsigned int index = 0; index < AREA; index++) {
		if (getPlane(data, 0, index) >= 0.5f) {
			board[index] = '#';
		} else if (getPlane(data, 1, index) >= 0.5f) {
			board[index] = 'X';
		} else if (getPlane(data, 2, index) >= 0.5f) {
			board[index] = 'O';
		}
	}

	return GameState::newGame('X', board);
}
