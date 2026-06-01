#ifndef BITBURNER_BOARD_GEN_H
#define BITBURNER_BOARD_GEN_H

#include <random>
#include <string>

/**
 * @brief Builds a board like BitBurner getNewBoardState with applyObstacles (no handicap), then random X/O on empty cells.
 * @param rng random number generator
 * @return flat board string (column-major, index = x * SIDE_LENGTH + y)
 */
std::string getNewBoardStateBitburner(std::mt19937_64& rng);

#endif
