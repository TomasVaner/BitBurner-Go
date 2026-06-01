#include <string>
#include <vector>

#include "utils.h"

std::vector<std::string> split(const std::string& str, const char delimiter, const bool allowEmpty) {
	std::vector<std::string> parts;
	
	std::string curPart;
	for (const char c : str) {
		if (c == delimiter) {
			if (!curPart.empty() || allowEmpty) {
				parts.push_back(curPart);
				curPart.clear();
			}
		} else {
			curPart.push_back(c);
		}
	}
	if (!curPart.empty()) {
		parts.push_back(curPart);
	}
	
	return parts;
}

Logger::Logger(const std::string& file_name)
	:file(file_name, std::ios::app)
{
}

void Logger::flush()
{
	file.flush();
}

template<>
Logger& Logger::operator<< (const char& arg)
{
	file << arg;
	if (options.console_return_caret != ReturnOpt::Pass && arg == '\n') {
		if (options.console_return_caret == ReturnOpt::ReplaceWithCaretOnce)
			options.console_return_caret = ReturnOpt::Pass;
		std::cout << '\r';
	}
	else {
		std::cout << arg;
	}
	return *this;
}

template<>
Logger& Logger::operator<< (const ReturnOpt& arg)
{
	options.console_return_caret = arg;
	return *this;
}