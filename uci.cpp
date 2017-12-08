#include "position.hpp"
#include "state.hpp"
#include "search.hpp"

static std::vector<std::string> split(std::string str){
	std::vector<std::string> ret;
	std::istringstream iss(str);
	std::string s;
	while (iss >> s) {
		ret.push_back(s);
	}
	return ret;
}
void uci_loop(){
	State state;
	Searcher searcher;
	while(true){
		std::string str;
		std::getline(std::cin, str);
		const std::vector<std::string> cmds = split(str);
		if(cmds.size() == 0 || cmds[0] == "quit")break;
		if(cmds[0] == "uci"){
			std::cout << "id name Cappuccino" << std::endl;
			std::cout << "id auther Watanabe Keisuke" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if(cmds[0] == "isready"){
			std::cout << "readyok" << std::endl;
		}
		else if(cmds[0] == "ucinewgame"){
		}
		else if(cmds[0] == "setoption"){
		}
		else if(cmds[0] == "position"){
			state.set_up(cmds);
		}
		else if(cmds[0] == "go"){
			searcher.go(state);
		}
		else if(cmds[0] == "stop"){
		}
		else if(cmds[0] == "ponderhit"){
		}
		//debug commands
		else if(cmds[0] == "genmove"){
			Array<Move, MaxLegalMove> moves;
			const Position& pos = state.pos();
			int n = pos.generate_important_moves(moves, 0);
			n = pos.generate_unimportant_moves(moves, n);
			std::cout << n << " ";
			for(int i=0;i<n;i++){
				if(pos.is_suicide_move(moves[i]))continue;
				std::cout << moves[i].to_fen() << " ";
			}
			std::cout << std::endl;
		}
		else if(cmds[0] == "eval"){
			std::cout << state.pos().evaluate() << std::endl;
		}
	}
}
