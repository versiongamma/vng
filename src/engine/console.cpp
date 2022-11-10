#include "console.h"

#include <iostream>
#include <imgui.h>

void Console::init() {
	
};

void Console::log(std::string msg) {
	std::cout << msg << std::endl;
	logMessages.push_back(msg);
}

void Console::draw() {
	ImGui::Begin("Console");
	ImGui::BeginChild("Log History", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() - 40), true, ImGuiWindowFlags_HorizontalScrollbar);
	for (auto msg : logMessages) {
		ImGui::Text(msg.c_str());
	}
	ImGui::EndChild();
	ImGui::End();
}

void Console::cleanup() {
	
}