#ifndef SPLIT_STRING_H
#define SPLIT_STRING_H
#include <vector>
#include <string>
#include <sstream>
inline std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}
#endif // SPLIT_STRING_H
