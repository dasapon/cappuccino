#include "position.hpp"
#include "move.hpp"

class State{
	std::vector<Position> positions;
	int ply;
public:
	const Position& pos()const{return positions[ply];}
	void unmake_move(){ply--;}
	void make_move(Move move){
		positions[ply+1] = positions[ply];
		ply++;
		positions[ply].make_move(move);
	}
	void init(const FEN& fen){
		ply = 0;
		if(positions.size() < 128)positions.resize(ply + 200);
		positions[0] = Position(fen);
	}
	void set_up(const std::vector<std::string>& cmds){
		int i = 1;
		FEN fen;
		if(cmds[i++] == "startpos"){
			fen = startpos;
		}
		else {
			assert(cmds[1] == "fen");
			assert(cmds.size() >= 2 + FEN::size());
			for(int j=0;j<FEN::size();j++){
				fen[j] = cmds[i++];
			}
		}
		init(fen);
		if(cmds.size() > i && cmds[i++]=="moves"){
			for(;i<cmds.size();i++){
				if(positions.size() < ply + 128){
					positions.resize(ply + 200);
				}
				make_move(pos().str2move(cmds[i]));
			}
		}
	}
};
