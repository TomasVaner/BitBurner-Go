#include "BoardAnalytics.h"

#include "GameStateConstants.h"

#include <queue>
#include <unordered_set>

namespace {

char flipColor(char color) {
	return color == 'O' ? 'X' : 'O';
}

ControlledEmpty floodControlledEmpty(const std::string& board, const char myColor, const char enemyColor) {
	ControlledEmpty result;
	result.myControlled.assign(AREA, 0.0f);
	result.enemyControlled.assign(AREA, 0.0f);

	auto visited = std::vector<bool>(AREA, false);
	for (unsigned int i = 0; i < AREA; i++) {
		if (board[i] != '.' || visited[i]) {
			continue;
		}

		std::queue<unsigned int> stack;
		stack.push(i);
		std::vector<unsigned int> region;
		bool touchesMy = false;
		bool touchesEnemy = false;

		while (!stack.empty()) {
			const unsigned int cur = stack.front();
			stack.pop();
			if (visited[cur]) {
				continue;
			}
			visited[cur] = true;

			if (board[cur] == myColor) {
				touchesMy = true;
				continue;
			}
			if (board[cur] == enemyColor) {
				touchesEnemy = true;
				continue;
			}
			if (board[cur] != '.') {
				continue;
			}

			region.push_back(cur);
			for (const unsigned int neighbor : NEIGHBORS[cur]) {
				if (!visited[neighbor]) {
					stack.push(neighbor);
				}
			}
		}

		if (touchesMy && !touchesEnemy) {
			for (const unsigned int cell : region) {
				result.myControlled[cell] = 1.0f;
			}
		} else if (touchesEnemy && !touchesMy) {
			for (const unsigned int cell : region) {
				result.enemyControlled[cell] = 1.0f;
			}
		}
	}

	return result;
}

void floodStoneChain(
	const std::string& board,
	const unsigned int start,
	const char chainColor,
	std::vector<bool>& visited,
	std::vector<unsigned int>& chain) {
	std::queue<unsigned int> stack;
	stack.push(start);
	visited[start] = true;
	chain.push_back(start);

	while (!stack.empty()) {
		const unsigned int cur = stack.front();
		stack.pop();
		for (const unsigned int neighbor : NEIGHBORS[cur]) {
			if (!visited[neighbor] && board[neighbor] == chainColor) {
				visited[neighbor] = true;
				chain.push_back(neighbor);
				stack.push(neighbor);
			}
		}
	}
}

unsigned int countChainLiberties(const std::string& board, const std::vector<unsigned int>& chain) {
	std::unordered_set<unsigned int> liberties;
	for (const unsigned int stone : chain) {
		for (const unsigned int neighbor : NEIGHBORS[stone]) {
			if (board[neighbor] == '.') {
				liberties.insert(neighbor);
			}
		}
	}
	return liberties.size();
}

void assignLibertyUrgency(
	const std::string& board,
	const char chainColor,
	std::vector<float>& urgency) {
	auto visited = std::vector<bool>(AREA, false);
	for (unsigned int i = 0; i < AREA; i++) {
		if (visited[i] || board[i] != chainColor) {
			continue;
		}

		std::vector<unsigned int> chain;
		floodStoneChain(board, i, chainColor, visited, chain);
		const unsigned int libertyCount = countChainLiberties(board, chain);
		if (libertyCount == 0) {
			continue;
		}
		const float value = 1.0f / static_cast<float>(libertyCount);
		for (const unsigned int stone : chain) {
			urgency[stone] = value;
		}
	}
}

} // namespace

ControlledEmpty computeControlledEmpty(const std::string& board, const char currentPlayer) {
	return floodControlledEmpty(board, currentPlayer, flipColor(currentPlayer));
}

LibertyUrgency computeLibertyUrgency(const std::string& board, const char currentPlayer) {
	LibertyUrgency result;
	result.myUrgency.assign(AREA, 0.0f);
	result.enemyUrgency.assign(AREA, 0.0f);
	assignLibertyUrgency(board, currentPlayer, result.myUrgency);
	assignLibertyUrgency(board, flipColor(currentPlayer), result.enemyUrgency);
	return result;
}
