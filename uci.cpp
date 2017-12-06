#include "position.hpp"

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
	Position pos;
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
			int i = 1;
			if(cmds[i++] == "startpos"){
				pos.load_fen(startpos);
			}
			else {
				assert(cmds[1] == "fen");
				FEN fen;
				assert(cmds.size() >= 2 + FEN::size());
				for(int j=0;i<FEN::size();j++){
					fen[j] = cmds[i++];
				}
				pos.load_fen(fen);
			}
			if(cmds.size() > i && cmds[i++]=="moves"){
				for(;i<cmds.size();i++){
					pos.make_move(pos.str2move(cmds[i]));
				}
			}
		}
		else if(cmds[0] == "go"){
		
		}
		else if(cmds[0] == "stop"){
		}
		else if(cmds[0] == "ponderhit"){
		}
		//debug commands
		else if(cmds[0] == "genmove"){
			pos.to_fen();
			Array<Move, MaxLegalMove> moves;
			int n = pos.generate_important_moves(moves, 0);
			std::cout << n <<"!!!"<< std::endl;
			n = pos.generate_unimportant_moves(moves, n);
			for(int i=0;i<n;i++){
				std::cout << moves[i].to_fen() << " ";
			}
			std::cout << std::endl;
		}
		else if(cmds[0] == "perft"){
			std::cout << performance_test(pos, 1) <<",";
			std::cout << performance_test(pos, 2) <<",";
			std::cout << performance_test(pos, 3) <<",";
			std::cout << performance_test(pos, 4) <<std::endl;
		}
	}
}
