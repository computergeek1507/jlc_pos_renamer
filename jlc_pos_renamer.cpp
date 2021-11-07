// jlc_pos_renamer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>

struct Correction {
	Correction(std::string pattern_, int rotation_) :
		pattern(std::move(pattern_)), rotation(rotation_) {}
	std::string const pattern;
	int const rotation;
};

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos) {
		return false;
	}
	str.replace(start_pos, from.length(), to);
	return true;
}

std::vector<std::string> split(std::string const& s, std::string const& delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

std::vector<Correction> ReadPartDataBase() {
	std::vector<Correction> parts;
	if (!std::filesystem::exists("cpl_rotations_db.csv")) {
		return parts;
	}

	std::fstream inFile;
	inFile.open("cpl_rotations_db.csv", std::ios::in);

	if (inFile.is_open()) {
		std::string line;
		getline(inFile, line);//get header
		while (getline(inFile, line)) {
			if (line.starts_with("#")) {
				continue;
			}

			auto const& lineData = split(line, ",");
			if (lineData.size() == 2) {
				auto regex = lineData[0];
				if (regex.size() < 2) { continue; }
				//regex.erase(0, 1);
				//regex.erase(regex.size() - 1);
				regex = regex.substr(1, regex.size() - 2);
				parts.emplace_back(regex, std::stoi(lineData[1]));
			}
		}
		inFile.close();
	}
	return parts;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
	// We print argv[0] assuming it is the program name
		std::cout << "not enought commandline args\n";
		return 1;
	}
	std::vector<Correction> part_db = ReadPartDataBase();
	std::string const fileName{ argv[1] };
	std::string newfileName{ fileName };
	replace(newfileName,"pos.csv", "jlc-pos.csv" );

	//if(std::filesystem::exists(newfileName)){}
	std::fstream outFile;

	outFile.open(newfileName, std::ios::out);
	std::fstream inFile;
	inFile.open(fileName, std::ios::in);

	if(inFile.is_open()) {
		std::string line;
		while (getline(inFile, line)) {
			
			//Ref,Val,Package,PosX,PosY,Rot,Side
			replace(line, "Ref,Val,Package,PosX,PosY,Rot,Side",
						"Designator,Val,Package,Mid X,Mid Y,Rotation,Layer");
			//replace(tp, "Ref,", "Designator,");
			//replace(tp, "PosX,", "Mid X,");
			//replace(tp, "PosY,", "Mid Y,");
			//replace(tp, "Rot,", "Rotation,");
			//replace(tp, "Side", "Layer");

			//rotate footprints
			auto const& lineData = split(line, ",");
			if (lineData.size() == 7) {
				auto package = lineData[2];
				if (package.size() >= 2) {
					package = package.substr(1, package.size() - 2);
				}
				for (auto const& map : part_db) {
					if (std::regex_match(package, std::regex(map.pattern)))	{
						int rot = std::stoi(lineData[5]);
						rot += map.rotation;
						rot = rot % 360;
						line = lineData[0] + "," + lineData[1] + "," + lineData[2] + "," + lineData[3] + ","
							+ lineData[4] + "," + std::to_string((float)rot) + "," + lineData[6];
					}
				}
			}

			outFile << line << "\n";
			std::cout << line << "\n";
		}
		inFile.close();
	}
	outFile.close();

	return 0;
}