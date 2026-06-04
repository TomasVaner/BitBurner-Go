#include "Example.h"

#include <format>
#include <unordered_map>

#include "GameStateData.h"
#include "../game/GameStateConstants.h"
#include "../utils.h"

namespace {

std::string stateKeyFromData(const std::vector<float>& gameStateData) {
	std::string state;
	state.reserve(GAME_STATE_DATA_LENGTH * 8);
	for (unsigned int i = 0; i < GAME_STATE_DATA_LENGTH; i++) {
		if (i > 0) {
			state.push_back(' ');
		}
		state += std::format("{:.4f}", gameStateData.at(i));
	}
	return state;
}

void readGameStateFromElements(const std::vector<std::string>& elements, std::vector<float>& gameStateData) {
	gameStateData.clear();
	gameStateData.reserve(GAME_STATE_DATA_LENGTH);
	for (unsigned int i = 0; i < GAME_STATE_DATA_LENGTH; i++) {
		gameStateData.push_back(std::stof(elements.at(i)));
	}
}

} // namespace

std::vector<Example> Example::load(const std::vector<std::tuple<std::vector<float>, std::vector<float>, float>>& turnInformation) {
	std::vector<Example> examples;
	examples.reserve(turnInformation.size());
	for (const auto& turn : turnInformation) {
		Example example;
		example.gameStateData = std::get<0>(turn);
		example.moveProbabilities = std::get<1>(turn);
		example.value = std::get<2>(turn);
		examples.push_back(example);
	}
	return examples;
}

std::vector<Example> Example::load(const std::vector<std::pair<std::vector<float>, std::vector<float>>>& turnInformation, const float value) {
	std::vector<Example> examples;
	examples.reserve(turnInformation.size());
	for (const auto& [gameStateData, moveProbabilities] : turnInformation) {
		Example example;
		example.gameStateData = gameStateData;
		example.moveProbabilities = moveProbabilities;
		example.value = value;
		examples.push_back(example);
	}
	return examples;
}

std::vector<Example> Example::load(std::istream& in) {
	std::vector<Example> examples;
	while (!(in.eof() || in.fail())) {
		Example example;
		std::string line;
		std::getline(in, line);
		if (in.fail()) {
			break;
		}
		if (line.empty()) {
			continue;
		}

		const std::vector<std::string> elements = split(line, ' ');
		if (elements.size() < GAME_STATE_DATA_LENGTH + 2) {
			continue;
		}

		readGameStateFromElements(elements, example.gameStateData);
		for (unsigned int i = GAME_STATE_DATA_LENGTH; i < elements.size() - 1; i++) {
			example.moveProbabilities.push_back(std::stof(elements.at(i)));
		}
		example.value = std::stof(elements.at(elements.size() - 1));
		examples.push_back(example);
	}
	return examples;
}

bool Example::safeLoad(std::istream& in, std::vector<Example>& examples) {
	constexpr int MAX_EXAMPLES = 65536;
	int count = 0;
	while (!(in.eof() || in.fail()) && count < MAX_EXAMPLES) {
		Example example;
		std::string line;
		std::getline(in, line);
		if (in.fail()) {
			break;
		}
		if (line.empty()) {
			continue;
		}

		const std::vector<std::string> elements = split(line, ' ');
		if (elements.size() < GAME_STATE_DATA_LENGTH + 2) {
			continue;
		}

		readGameStateFromElements(elements, example.gameStateData);
		for (unsigned int i = GAME_STATE_DATA_LENGTH; i < elements.size() - 1; i++) {
			example.moveProbabilities.push_back(std::stof(elements.at(i)));
		}
		example.value = std::stof(elements.at(elements.size() - 1));
		examples.push_back(example);
		count++;
	}
	return (in.eof() || in.fail());
}

void Example::save(std::ostream& out, const std::vector<Example>& examples) {
	out << std::fixed;
	for (const Example& example : examples) {
		for (unsigned int i = 0; i < example.gameStateData.size(); i++) {
			out << example.gameStateData.at(i) << ' ';
		}
		for (const float moveProbability : example.moveProbabilities) {
			if (moveProbability == ceilf(moveProbability)) {
				out << static_cast<int>(moveProbability) << ' ';
			} else {
				out << moveProbability << ' ';
			}
		}
		if (example.value == ceilf(example.value)) {
			out << static_cast<int>(example.value);
		} else {
			out << example.value;
		}
		out << '\n';
	}
	out.flush();
	out << std::defaultfloat;
}

void Example::saveAverage(std::istream& in, std::ostream& out) {
	std::unordered_map<std::string, std::vector<std::pair<std::vector<float>, float>>> examples;
	while (!(in.eof() || in.fail())) {
		std::string line;
		std::getline(in, line);
		if (in.fail()) {
			break;
		}
		if (line.empty()) {
			continue;
		}

		const std::vector<std::string> elements = split(line, ' ');
		if (elements.size() < GAME_STATE_DATA_LENGTH + 2) {
			continue;
		}

		std::vector<float> stateData;
		readGameStateFromElements(elements, stateData);
		const std::string state = stateKeyFromData(stateData);

		std::pair<std::vector<float>, float> result;
		for (unsigned int i = GAME_STATE_DATA_LENGTH; i < elements.size() - 1; i++) {
			result.first.emplace_back(std::stof(elements.at(i)));
		}
		result.second = std::stof(elements.at(elements.size() - 1));

		if (!examples.contains(state)) {
			examples.emplace(state, std::vector<std::pair<std::vector<float>, float>>{});
		}
		examples.at(state).push_back(result);
	}

	out << std::fixed;
	for (const auto& [stateKey, results] : examples) {
		out << stateKey << ' ';

		std::vector<float> totalProbabilities(NUM_MOVES, 0.0f);
		float totalValue = 0.0f;
		for (const auto& [moveProbabilities, value] : results) {
			for (size_t move = 0; move < moveProbabilities.size(); move++) {
				totalProbabilities.at(move) += moveProbabilities.at(move);
			}
			totalValue += value;
		}

		for (float moveProbability : totalProbabilities) {
			moveProbability /= static_cast<float>(results.size());
			if (moveProbability == ceilf(moveProbability)) {
				out << static_cast<int>(moveProbability) << ' ';
			} else {
				out << moveProbability << ' ';
			}
		}
		const float value = totalValue / static_cast<float>(results.size());
		if (value == ceilf(value)) {
			out << static_cast<int>(value);
		} else {
			out << value;
		}
		out << '\n';
	}
	out.flush();
	out << std::defaultfloat;
}

std::vector<float>& Example::getGameStateData() {
	return gameStateData;
}

std::vector<float> Example::getMoveProbabilities() {
	return moveProbabilities;
}

float Example::getValue() const {
	return value;
}

void Example::display(std::ostream& out) const {
	GameState* gameState = getGameState(gameStateData);
	out << *gameState;

	for (const int move : *gameState->getValidMoves()) {
		out << move << " " << moveProbabilities.at(move + 1) << '\n';
	}

	out << value << '\n';

	delete gameState;
}
