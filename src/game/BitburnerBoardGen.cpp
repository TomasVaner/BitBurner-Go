#include "BitburnerBoardGen.h"

#include "GameStateConstants.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

namespace {

int randomInt(std::mt19937_64& rng, int n1, int n2) {
	std::uniform_real_distribution distribution(0.0, 1.0);
	return n1 + static_cast<int>(std::floor((n2 - n1 + 1) * distribution(rng)));
}

char getCell(const std::string& board, unsigned int x, unsigned int y) {
	return board[x * SIDE_LENGTH + y];
}

void setCell(std::string& board, unsigned int x, unsigned int y, char cell) {
	board[x * SIDE_LENGTH + y] = cell;
}

bool isPlayable(char cell) {
	return cell != '#';
}

int getScale() {
	// boardSizes.indexOf(SIDE_LENGTH) for [5, 7, 9, 13, 19]
	switch (SIDE_LENGTH) {
	case 5: return 0;
	case 7: return 1;
	case 9: return 2;
	case 13: return 3;
	case 19: return 4;
	default: return 0;
	}
}

std::string rotate90Degrees(const std::string& board) {
	std::string result(AREA, '.');
	for (unsigned int newX = 0; newX < SIDE_LENGTH; newX++) {
		std::vector<char> column;
		column.reserve(SIDE_LENGTH);
		for (unsigned int oldX = 0; oldX < SIDE_LENGTH; oldX++) {
			column.push_back(getCell(board, oldX, newX));
		}
		std::reverse(column.begin(), column.end());
		for (unsigned int y = 0; y < SIDE_LENGTH; y++) {
			setCell(result, newX, y, column[y]);
		}
	}
	return result;
}

std::string rotateNTimes(std::string board, int rotations) {
	for (int i = 0; i < rotations; i++) {
		board = rotate90Degrees(board);
	}
	return board;
}

std::string randomizeRotation(std::string board, std::mt19937_64& rng) {
	return rotateNTimes(std::move(board), randomInt(rng, 0, 3));
}

void addDeadCorner(std::string& board, std::mt19937_64& rng, int size) {
	int currentSize = size;
	for (int i = 0; i < size && i < currentSize; i++) {
		if (randomInt(rng, 0, 1)) {
			currentSize--;
		}
		for (unsigned int y = 0; y < SIDE_LENGTH; y++) {
			if (static_cast<int>(y) < currentSize && isPlayable(getCell(board, i, y))) {
				setCell(board, i, y, '#');
			}
		}
	}
}

std::string addDeadCorners(std::string board, std::mt19937_64& rng) {
	const int scale = getScale() + 1;
	addDeadCorner(board, rng, scale);
	if (!randomInt(rng, 0, 3)) {
		board = rotate90Degrees(board);
		board = rotate90Degrees(board);
		addDeadCorner(board, rng, scale - 2);
	}
	return randomizeRotation(std::move(board), rng);
}

std::string addCenterBreak(std::string board, std::mt19937_64& rng) {
	const int maxOffset = getScale();
	const int xIndex = randomInt(rng, 0, maxOffset * 2) - maxOffset + static_cast<int>(SIDE_LENGTH / 2);
	const int length = randomInt(rng, 1, static_cast<int>(std::floor(SIDE_LENGTH / 2.0)) - 1);
	for (unsigned int y = 0; y < SIDE_LENGTH; y++) {
		if (static_cast<int>(y) < length) {
			setCell(board, static_cast<unsigned int>(xIndex), y, '#');
		}
	}
	return randomizeRotation(std::move(board), rng);
}

std::string removeRows(std::string board, std::mt19937_64& rng) {
	const int rowsToRemove = std::max(randomInt(rng, -2, getScale()), 1);
	for (int i = 0; i < rowsToRemove; i++) {
		for (unsigned int y = 0; y < SIDE_LENGTH; y++) {
			setCell(board, static_cast<unsigned int>(i), y, '#');
		}
	}
	return rotateNTimes(std::move(board), 3);
}

std::string addDeadNodesToEdge(std::string board, std::mt19937_64& rng, int maxPerEdge) {
	for (int i = 0; i < 4; i++) {
		const int count = randomInt(rng, 0, maxPerEdge);
		for (int j = 0; j < count; j++) {
			const int yIndex = std::max(randomInt(rng, -2, static_cast<int>(SIDE_LENGTH) - 1), 0);
			setCell(board, 0, static_cast<unsigned int>(yIndex), '#');
		}
		board = rotate90Degrees(board);
	}
	return board;
}

std::string ensureOfflineNodes(std::string board) {
	for (char cell : board) {
		if (cell == '#') {
			return board;
		}
	}
	setCell(board, 0, 0, '#');
	return board;
}

std::string removeIslands(std::string board) {
	std::vector<bool> visited(AREA, false);

	for (unsigned int start = 0; start < AREA; start++) {
		if (visited[start] || board[start] != '.') {
			continue;
		}

		std::vector<unsigned int> chain;
		std::queue<unsigned int> queue;
		queue.push(start);
		visited[start] = true;

		while (!queue.empty()) {
			const unsigned int index = queue.front();
			queue.pop();
			chain.push_back(index);

			for (const unsigned int neighbor : NEIGHBORS[index]) {
				if (!visited[neighbor] && board[neighbor] == '.') {
					visited[neighbor] = true;
					queue.push(neighbor);
				}
			}
		}

		if (chain.size() <= 2) {
			for (const unsigned int index : chain) {
				board[index] = '#';
			}
		}
	}

	return board;
}

void addObstacles(std::string& board, std::mt19937_64& rng) {
	const bool shouldRemoveCorner = !randomInt(rng, 0, 4);
	const bool shouldRemoveRows = !shouldRemoveCorner && !randomInt(rng, 0, 4);
	const bool shouldAddCenterBreak = !shouldRemoveCorner && !shouldRemoveRows && randomInt(rng, 0, 3);
	const int obstacleTypeCount = static_cast<int>(shouldRemoveCorner) + static_cast<int>(shouldRemoveRows) +
		static_cast<int>(shouldAddCenterBreak);

	int edgeDeadMax = static_cast<int>((getScale() + 2 - obstacleTypeCount) * 1.5);
	if (edgeDeadMax < 1) {
		edgeDeadMax = 1;
	}
	const int edgeDeadCount = randomInt(rng, 1, edgeDeadMax);

	if (shouldRemoveCorner) {
		board = addDeadCorners(std::move(board), rng);
	}

	if (shouldAddCenterBreak) {
		board = addCenterBreak(std::move(board), rng);
	}

	board = randomizeRotation(std::move(board), rng);

	if (shouldRemoveRows) {
		board = removeRows(std::move(board), rng);
	}

	board = addDeadNodesToEdge(std::move(board), rng, edgeDeadCount);
	board = ensureOfflineNodes(std::move(board));
	board = removeIslands(std::move(board));
}

void scatterRandomStones(std::string& board, std::mt19937_64& rng) {
	std::uniform_real_distribution distribution(0.0, 1.0);
	for (unsigned int i = 0; i < AREA; i++) {
		if (board[i] != '.') {
			continue;
		}
		const double value = distribution(rng);
		if (value <= 0.001) {
			board[i] = 'X';
		} else if (value <= 0.003) {
			board[i] = 'O';
		}
	}
}
} // namespace


std::string getNewBoardStateBitburner(std::mt19937_64& rng) {
	std::string board(AREA, '.');
	addObstacles(board, rng);
	scatterRandomStones(board, rng);
	return board;
}
