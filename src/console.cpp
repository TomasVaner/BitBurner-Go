#include <iostream>

#include "utils.h"
#include "game/GameState.h"
#include "ai/BasicMCTS.h"
#include "ai/AdvancedMCTS.h"

int main(int argc, char* argv[]) {
	std::vector<std::shared_ptr<MCTS>> mcts;
	std::vector<NeuralNetwork> neuronNetworks;

	for (int i = 1; i < argc; i += 2)
	{
		if (strncmp(argv[i], "basic", 5) == 0)
		{
			mcts.push_back(std::make_shared<BasicMCTS>(std::stoi(argv[i + 1])));
		}
		else
		{
			neuronNetworks.emplace_back();
			neuronNetworks.back().load(argv[i]);
			mcts.push_back(std::make_shared<AdvancedMCTS>(&neuronNetworks.back(), std::stoi(argv[i + 1])));
		}
	}

	auto rng = std::mt19937_64(std::random_device{}());
	
	bool keep_play = true;
	while (keep_play)
	{
		auto gameState = GameState::newGame('X', GameState::getRandomBoard(rng));
		gameState->printGameState();

		while (gameState->getEndState() < -1) {
			std::vector<std::vector<float>> moveProbabilities;
			moveProbabilities.reserve(mcts.size());
			for (const auto& m : mcts)
				moveProbabilities.emplace_back(m->getMoveProbabilities(gameState));

			std::cout << std::left;
			std::cout << std::setw(3) << "i"
					  << std::setw(7) << "M";
		
			for (size_t i = 0; i < moveProbabilities.size(); i++) 
				std::cout << std::setw(4) << "P " << std::setw(1) << i + 1
						  << std::setw(4) << "V " << std::setw(1) << i + 1;
			std::cout << '\n';
		
			for (unsigned int i = 0; i < gameState->getValidMoves()->size(); i++) {
				const auto move = gameState->getValidMoves()->at(i);
				const auto x = move / SIDE_LENGTH;
				const auto y = move % SIDE_LENGTH;
				std::cout << std::setw(3) << i
						  << std::setw(3) << move;
				if (move >= 0)
				{
					std::cout << std::setw(2) << x
							  << std::setw(2) << y;
				}
				else {
					std::cout << "----";
				}
			
				for (size_t p = 0; p < moveProbabilities.size(); p++)
				{
					std::cout << std::setw(5) << moveProbabilities[p][gameState->getValidMoves()->at(i) + 1]
							  << std::setw(12) << mcts[p]->getMoveValue(gameState->getChild(i));
				}
				std::cout << '\n';
			}
			
			std::string command;
			std::getline(std::cin, command);
			int moveToPlay = 0;
			if (command == "exit")
			{
				keep_play = false;
				break;
			}
			else if (command == "reset")
			{
				break;
			}
			else if (command.starts_with("move"))
			{
				auto m = split(command, ' ');
				if (m.size() == 2)
				{
					moveToPlay = std::stoi(m[1]);
				}
				else if (m.size() == 3)
				{
					auto x = std::stoi(m[1]);
					auto y = std::stoi(m[2]);
					auto cell_id = x * SIDE_LENGTH + y;
					auto valid_move = std::ranges::find(*gameState->getValidMoves(), cell_id);
					if (valid_move == gameState->getValidMoves()->end())
						std::cout << "Invalid move: " << x << ", " << y << '\n';
					moveToPlay = std::distance(gameState->getValidMoves()->begin(), valid_move);
				}
			} else if (command != "skip")
			{
				std::cout << "Unknown command: '" << command << "'\n";
			}

			auto childGameState = gameState->getChild(moveToPlay, false);
			delete gameState;
			gameState = childGameState;

			gameState->printGameState();
		
			for (auto& m : mcts)
				m->reset();
		}

		std::cout << gameState->getEndState() << '\n';
		delete gameState;
	}


	return 0;
}