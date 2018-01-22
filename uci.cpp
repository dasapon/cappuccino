#include "state.hpp"
#include "search.hpp"
#include "evaluate.hpp"

std::vector<std::string> split(std::string str){
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
		if(cmds.size() == 0 || cmds[0] == "quit"){
			searcher.stop();
			break;
		}
		if(cmds[0] == "uci"){
			std::cout << "id name Cappuccino 0.1.0" << std::endl;
			std::cout << "id auther Watanabe Keisuke" << std::endl;
			std::cout << "option name Hash type spin default 32 min 1 max 8192" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if(cmds[0] == "isready"){
			std::cout << "readyok" << std::endl;
		}
		else if(cmds[0] == "ucinewgame"){
		}
		else if(cmds[0] == "setoption"){
			if(cmds.size() == 5 && cmds[1] == "name" && cmds[3] == "value"){
				if(cmds[2] == "Hash"){
					searcher.hash_size(std::stoi(cmds[4]));
				}
			}
		}
		else if(cmds[0] == "position"){
			state.set_up(cmds);
		}
		else if(cmds[0] == "go"){
			Array<uint64_t, PlayerDim> time({0,0}), inc({0,0});
			bool ponder_or_infinite = false;
			for(int i=1;i<cmds.size();i++){
				if(cmds[i] == "ponder" || cmds[i] == "infinite"){
					ponder_or_infinite = true;
				}
				else if(cmds[i] == "wtime")time[White] = std::stoi(cmds[i+1]);
				else if(cmds[i] == "btime")time[Black] = std::stoi(cmds[i+1]);
				else if(cmds[i] == "winc")inc[White] = std::stoi(cmds[i+1]);
				else if(cmds[i] == "binc")inc[Black] = std::stoi(cmds[i+1]);
			}
			Player turn = state.pos().turn_player();
			searcher.go(state, time[turn], inc[turn], ponder_or_infinite);
		}
		else if(cmds[0] == "stop"){
			searcher.stop();
		}
		else if(cmds[0] == "ponderhit"){
			searcher.ponder_hit();
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
				std::cout << moves[i].to_lan() << " " << pos.is_move_check(moves[i]) << " ";
			}
			std::cout << std::endl;
		}
		else if(cmds[0] == "eval"){
			std::cout << evaluate(state) << std::endl;
		}
		else if(cmds[0] == "key"){
			std::cout << "hashkey = " << state.pos().key() << std::endl;
			std::cout << state.draw() << std::endl;
		}
		else if(cmds[0] == "piece_list"){
			int n;
			const Array<int, 32>& list = state.get_piece_list(&n);
			for(int i=0;i<n;i++)std::cout << list[i] << " ";
			std::cout << std::endl;
		}
	}
}
