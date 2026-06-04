#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <iostream>
#include <vector>

class Example {
public:
	static std::vector<Example> load(const std::vector<std::tuple<std::vector<float>, std::vector<float>, float>>& turnInformation);
	static std::vector<Example> load(const std::vector<std::pair<std::vector<float>, std::vector<float>>>& turnInformation, float value);
	static std::vector<Example> load(std::istream& in);
	static bool safeLoad(std::istream& in, std::vector<Example>& examples);
	static void save(std::ostream& out, const std::vector<Example>& examples);
	static void saveAverage(std::istream& in, std::ostream& out);

	std::vector<float>& getGameStateData();
	std::vector<float> getMoveProbabilities();
	[[nodiscard]] float getValue() const;
	void display(std::ostream& out) const;
private:
	std::vector<float> gameStateData;
	std::vector<float> moveProbabilities;
	float value = -3;
};

#endif
