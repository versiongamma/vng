#pragma once

#include <sstream>
#include <functional>

class Console {
public:
	void init();
	void cleanup();
    void log(std::string msg);
	void draw();

private:
	std::vector<std::string> logMessages;
};