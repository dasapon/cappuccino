#pragma once

#include "position.hpp"
#include "move.hpp"

class State{
	class CurState{
	public:
		Position pos;
		PieceList piece_list;
		Move last_move;
		CurState(){};
		CurState(const Position& pos):pos(pos), last_move(NullMove){
			pos.piece_list(piece_list);
		}
		void set(const Position& pos, Move move){
			this->pos = pos;
			this->pos.make_move(move);
			last_move = move;
			this->pos.piece_list(piece_list);
		}
		void operator=(const CurState& cs){
			pos = cs.pos;
			piece_list = cs.piece_list;
			last_move = cs.last_move;
		}
	};
	std::vector<CurState> history;
	int ply;
public:
	void operator=(const State& state){
		ply = state.ply;
		for(int i=0;i<ply;i++)history[i] = state.history[i];
	}
	const Position& pos()const{return history[ply].pos;}
	const PieceList& piece_list()const{
		return history[ply].piece_list;
	}
	float progress()const{
		return 1.0f - (history[ply].piece_list.size - 2.0f) / 30.0f;
	}
	Move previous_move(int i)const{
		if(ply < i)return NullMove;
		return history[ply - i].last_move;
	}
	void unmake_move(){ply--;}
	void make_move(Move move){
		history[ply+1].set(pos(), move);
		ply++;
	}
	void unmake_all_move(){ply = 0;}
	void init(const FEN& fen){
		ply = 0;
		if(history.size() < 2048)history.resize(2048);
		history[0] = Position(fen);
	}
	bool draw()const{
		if(pos().immediately_draw())return true;
		uint64_t key = pos().key();
		for(int i = ply-1, e = std::max(0, ply - 16);i >= e; i--){
			if(history[i].pos.key() == key)return true;
		}
		return false;
	}
	State(){init(startpos);}
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
				make_move(Move(pos(), cmds[i]));
			}
		}
	}
	int game_ply()const{return ply;}
};
