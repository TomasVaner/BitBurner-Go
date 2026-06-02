#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Splits a string by a given delimiter
 * @param str string to split
 * @param delimiter delimiter to split string by
 * @param allowEmpty whether empty strings should be kept
 * @return vector with all the parts of the string split by the given delimiter
 */
std::vector<std::string> split(const std::string& str, const char delimiter, bool allowEmpty = false);

class Logger
{
public:
	Logger(const std::string& file_name);

	template<typename T>
	Logger& operator<< (const T& arg)
	{
		file << arg;
		if (options.cout_ignore == IgnoreCoutOpt::Keep)
			std::cout << arg;
		return *this;
	}
	void flush();

	enum class ReturnOpt
	{
		ReplaceWithCaretOnce,
		ReplaceWithCaret,
		Pass
	};

	enum class IgnoreCoutOpt
	{
		Keep,
		Ignore,
	};

private:
	std::ofstream file;
	struct Options
	{
		ReturnOpt console_return_caret = ReturnOpt::Pass;
		IgnoreCoutOpt cout_ignore = IgnoreCoutOpt::Keep;
	} options;
};

template<>
Logger& Logger::operator<< (const char& arg);

template<>
Logger& Logger::operator<< (const ReturnOpt& arg);

template<>
Logger& Logger::operator<< (const IgnoreCoutOpt& arg);

#endif