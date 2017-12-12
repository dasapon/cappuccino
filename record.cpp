#include <fstream>
#include <regex>

#include "move.hpp"
#include "position.hpp"

static std::vector<std::string> read_block(std::ifstream& ifs){
	std::string str;
	std::vector<std::string> ret;
	while(true){
		std::getline(ifs, str);
		if(str.size() == 0 || str == "\n" || str == "\r\n" || str == "\r")break;
		ret.push_back(str);
	}
	return ret;
}

std::vector<std::vector<Move>> read_pgn(std::string file_name){
	std::ifstream ifs(file_name);
	int n_record = 0;
	std::vector<std::vector<Move>> records;
	while(true){
		std::vector<std::string> tags = read_block(ifs);
		if(tags.size() == 0)break;
		std::vector<std::string> pgn_moves = read_block(ifs);
		int black_elo = 0, white_elo = 0;
		for(std::string tag : tags){
			if(tag.find("[BlackElo \"") == 0){
				tag = tag.substr(std::string("[BlackElo \"").size(), tag.size() - 2);
				black_elo = std::stoi(tag);
			}
			else if(tag.find("[WhiteElo \"")==0){
				tag = tag.substr(std::string("[WhiteElo \"").size(), tag.size() - 2);
				white_elo = std::stoi(tag);
			}
		}
		if(black_elo <= 2500 || white_elo <= 2500)continue;
		//parse moves
		Position pos;
		std::vector<Move> record;
		for(const std::string& line : pgn_moves){
			std::vector<std::string> v = split(line);
			std::regex rgx("\\d+.");
			for(const std::string& str : v){
				if(std::regex_match(str, rgx))continue;
				else if(str == "1-0" || str == "1/2-1/2" || str == "0-1")break;
				Move move = pgn2move(pos, str);
				if(move == NullMove){
					std::cout << str << std::endl;
					return records;
				}
				record.push_back(move);
				pos.make_move(move);
			}
		}
		records.push_back(record);
		if(++n_record%5000==0)std::cout << n_record<<std::endl;
	}
	ifs.close();
	return records;
}
