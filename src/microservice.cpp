#include "game/GameState.h"
#include "ai/AdvancedMCTS.h"
#include "ai/BasicMCTS.h"
#include "Listener.h"
#include "utils.h"

#include <iostream>


int main(int argc, char* argv[]) {
	auto const address = net::ip::make_address("0.0.0.0");
	constexpr unsigned short port = 8080;

	std::shared_ptr<MCTS> mcts;

	NeuralNetwork neuralNetwork;
	if (argc >= 3)
	{
		neuralNetwork.load(argv[1]);
		mcts = std::make_unique<AdvancedMCTS>(&neuralNetwork, std::stoi(argv[2]));
	}
	else {
		auto simulations = 5000;
		if (argc >= 2)
		{
			simulations = std::stoi(argv[1]);
		}

		mcts = std::make_shared<BasicMCTS>(simulations);
	}

	net::io_context ioc{1};
	std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, [mcts](const std::string& request) {
		try {
			std::vector<std::string> requestParts = split(request, ',');
			const char color = requestParts.at(0).at(0);
			const std::string& board = requestParts.at(1);
			std::vector<std::string> previousBoards;
			if (requestParts.size() > 2) {
				previousBoards = {requestParts.begin() + 2, requestParts.end()};
			}
			
			GameState* gameState = GameState::newGame(color, board, previousBoards);

			std::string response = std::to_string(gameState->getValidMoves()->at(mcts->getBestMove(gameState)));

			mcts->reset();
			delete gameState;

			return response;
		} catch (std::exception& e) {
			std::cout << "Request body formatted incorrectly." << '\n';
			std::cout << "Request: " << request << '\n';
			std::cout << "Exception: " << e.what() << '\n';

			return std::string("Request body formatted incorrectly.");
		}
	})->run();

	ioc.run();

	return 0;
}
