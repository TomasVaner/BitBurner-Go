#include "ai/AdvancedMCTS.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <chrono>
#include <random>
#include <ctime>
#include <atomic>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include "ai/NeuralNetwork.h"
#include "ai/MCTS.h"
#include "ai/Example.h"
#include "utils.h"

int main(int argc, char* argv[]) {
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	std::stringstream date_stream;
	date_stream << "training_" << std::put_time(&tm, "%Y%m%d%H%M%S");
	std::filesystem::path training_folder = date_stream.str();
	std::filesystem::create_directory(training_folder);

	Logger lout((training_folder / "trainerLog.txt").string());

	NeuralNetwork neuralNetwork;
	lout << "Starting up" << '\n';
	if (argc >= 2) {
		if (!neuralNetwork.load(argv[1])) {
			lout << "ERROR: Starting current model did not load correctly from " << argv[1] << '\n';
		}
	} else {
		lout << "WARNING: No model was passed." << '\n';
	}
	
	std::ifstream fin("config.txt");
	
	if (fin.fail()) {
		lout << "FATAL: Config file did not load correctly" << '\n';
		return 1;
	}
	
	std::vector<int> config;
	int iTemp;
	while (fin >> iTemp) {
		config.push_back(iTemp);
		fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	fin.close();

	if (config.size() < 11) {
		lout << "FATAL: Config file did not load correctly" << '\n';
		return 1;
	}
	
	const int NUM_ITERATIONS = config.at(0);
	const int NUM_EPISODES = config.at(1);
	const int NUM_SIMULATIONS = config.at(2);
	const int NUM_GAMES = config.at(3);
	const int EXPLORATION_TURNS = config.at(4);
	const int BATCH_SIZE = config.at(5);
	const int EPOCHS = config.at(6);
	const int SKIP_EXAMPLE_GENERATION = config.at(7);
	const int DISPLAY_GAMES = config.at(8);
	const int MAXIMUM_TURNS = config.at(9);
	const float RESULT_WEIGHT = config.at(10) / 100.0f;
	const int SELF_PLAY_WORKERS = config.size() > 11 ? std::max(1, config.at(11)) : 1;
	const int MAX_IN_FLIGHT_EPISODES = config.size() > 12 ? std::max(1, config.at(12)) : std::numeric_limits<int>::max();
	const unsigned int DETERMINISTIC_SEED_BASE = config.size() > 13 ? static_cast<unsigned int>(config.at(13)) : std::random_device{}();
	auto rng = std::mt19937_64(DETERMINISTIC_SEED_BASE);
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
		lout << "Starting iteration " << iteration << '\n';
		lout.flush();
		std::chrono::steady_clock::time_point selfPlayBegin = std::chrono::steady_clock::now();
		auto iter_str = std::format("{:0>3}", iteration);
		auto ex_path = training_folder / (iter_str + "_gameMCTSTemp.ex");
		auto multi_gm_path = training_folder / (iter_str + "_multiGameMCTSTemp.gm");

		if (SKIP_EXAMPLE_GENERATION == 0 || iteration != 0) {
			lout << "Starting example generation" << '\n';
			lout << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes have passed" << '\n';
			lout.flush();

			const int requestedWorkers = SELF_PLAY_WORKERS;
			const int effectiveWorkers = std::max(1, std::min({requestedWorkers, NUM_EPISODES, MAX_IN_FLIGHT_EPISODES}));

			struct EpisodeResult {
				std::vector<Example> examples;
				float result = 0.0f;
			};
			std::vector<EpisodeResult> episodeResults(NUM_EPISODES);
			std::atomic<int> finishedEpisodes = 0;

			std::vector<NeuralNetwork> workerNetworks(static_cast<size_t>(effectiveWorkers), neuralNetwork);

			boost::asio::thread_pool pool(effectiveWorkers);
			for (int worker = 0; worker < effectiveWorkers; worker++) {
				boost::asio::post(pool, [&, worker] {
					AdvancedMCTS workerMcts(&workerNetworks[static_cast<size_t>(worker)], NUM_SIMULATIONS);
					std::uniform_real_distribution distribution(0.0, 1.0);
					std::mt19937_64 workerRng(DETERMINISTIC_SEED_BASE + static_cast<unsigned int>(iteration * 10007 + worker * 379 + 17));

					for (int episode = worker; episode < NUM_EPISODES; episode += effectiveWorkers) {
						GameState* curGameState = GameState::newGame('O', GameState::getRandomBoard(workerRng));
						int turns = 0;
						std::vector<float> probabilities;
						std::vector<std::tuple<std::vector<uint8_t>, std::vector<float>, float>> turnInformation;
						while (curGameState->getEndState() < -1) {
							probabilities = workerMcts.getMoveProbabilities(curGameState);
							turnInformation.emplace_back(toVector(curGameState), probabilities, workerMcts.getMoveValue(curGameState));

							int moveNum = 0;
							if (turns < EXPLORATION_TURNS) {
								float total = 0;
								auto target = static_cast<float>(distribution(workerRng));
								for (int i = 0; i < curGameState->getValidMoves()->size(); i++) {
									total += probabilities[curGameState->getValidMoves()->at(i) + 1];
									if (target < total) {
										moveNum = i;
										break;
									}
								}
							} else {
								float highestProbability = -1;
								int bestMove = -1;
								for (int i = 0; i < curGameState->getValidMoves()->size(); i++) {
									float probability = probabilities[curGameState->getValidMoves()->at(i) + 1];
									if (probability > highestProbability) {
										highestProbability = probability;
										bestMove = i;
									}
								}
								moveNum = bestMove;
							}

							GameState* child = curGameState->getChild(moveNum, false);
							delete curGameState;
							curGameState = child;
							workerMcts.reset();
							turns++;
						}
						float result = curGameState->getEndState();
						for (auto& turnInfo : turnInformation) {
							std::get<2>(turnInfo) = std::get<2>(turnInfo) * (1 - RESULT_WEIGHT) + result * RESULT_WEIGHT;
						}
						episodeResults[episode].examples = Example::load(turnInformation);
						episodeResults[episode].result = result;
						delete curGameState;
						auto fe = finishedEpisodes.fetch_add(1);

						if (worker == effectiveWorkers - 1 || episode == NUM_EPISODES - 1)
							lout << "Finished " << (fe + 1) << " episode(s) in " << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - selfPlayBegin).count() << " minutes." << Logger::ReturnOpt::ReplaceWithCaretOnce << '\n';

					}
				});
			}
			pool.join();
			std::cout << '\n';

			if (finishedEpisodes.load() != NUM_EPISODES) {
				lout << "FATAL: Only finished " << finishedEpisodes.load() << " of " << NUM_EPISODES << " episodes" << '\n';
				return 1;
			}

			std::ofstream exout(ex_path, std::ios::app);
			std::ofstream gmout(multi_gm_path, std::ios::app);
			for (int episode = 0; episode < NUM_EPISODES; episode++) {
				Example::save(exout, episodeResults[episode].examples);
				gmout << std::fixed;
				gmout << episodeResults[episode].result << '\n';
				gmout << std::defaultfloat;
			}
			exout.flush();
			gmout.flush();
			exout.close();
			gmout.close();

			lout << "Finished " << NUM_EPISODES << " episode(s) in "
				 << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - selfPlayBegin).count()
				 << " minutes with " << effectiveWorkers << " worker(s)." << '\n';
			lout.flush();
		}
		const auto selfPlayMinutes = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - selfPlayBegin).count();

		fin.open(ex_path);

		auto average_ex_path = training_folder / (iter_str + "_average.ex");
		std::ofstream fout(average_ex_path);
		Example::saveAverage(fin, fout);
		fin.close();
		fout.close();

		std::vector<Example> examples;

		for (int epoch = 0; epoch < EPOCHS; epoch++) {
			fin.open(average_ex_path);
			examples = Example::load(fin);
			fin.close();

			std::shuffle(examples.begin(), examples.end(), rng);
			fout.open(average_ex_path);
			Example::save(fout, examples);
			fout.close();

			fin.open(average_ex_path);
			bool complete = false;
			while (!complete) {
				examples.clear();
				complete = Example::safeLoad(fin, examples);

				lout << "Training with " << examples.size() << " examples." << '\n';
				lout << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes have passed" << '\n';
				lout.flush();
				
				neuralNetwork.train(examples, BATCH_SIZE);
			}

			fin.close();
		}
		std::filesystem::create_directory(training_folder / "models");
		const auto tmp2_nn_path = training_folder / "models" / (iter_str + "_temp.pt");
		if (!neuralNetwork.save(tmp2_nn_path.string())) {
			lout << "ERROR: Current model did not save correctly to models/temp.pt" << '\n';
		}
		
		const auto totalMinutes = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - begin).count();
		const auto trainMinutes = totalMinutes - selfPlayMinutes;
		lout << "Validation: generated " << NUM_EPISODES << " episodes for iteration " << iteration << '\n';
		lout << "Timing split (minutes) selfPlay=" << selfPlayMinutes << " train=" << trainMinutes << " total=" << totalMinutes << '\n';
		lout << "Iteration " << iteration << " took " << std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-begin).count() << " minutes" << '\n';
		lout.flush();
	}
	
	return 0;
}