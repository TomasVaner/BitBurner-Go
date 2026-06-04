#ifndef BOARD_ANALYTICS_H
#define BOARD_ANALYTICS_H

#include <string>
#include <vector>

/**
 * @brief Per-cell controlled empty territory (1 on empty cells only).
 */
struct ControlledEmpty {
	std::vector<float> myControlled;
	std::vector<float> enemyControlled;
};

/**
 * @brief Per-cell liberty urgency 1/n for stones of each color (0 on non-stones).
 */
struct LibertyUrgency {
	std::vector<float> myUrgency;
	std::vector<float> enemyUrgency;
};

ControlledEmpty computeControlledEmpty(const std::string& board, char currentPlayer);
LibertyUrgency computeLibertyUrgency(const std::string& board, char currentPlayer);

#endif
