#pragma once

#include "position.hpp"

using PV = Array<Move, 128>;

class MoveOrderer{
	int n_moves;
	int idx;
	Array<Move, MaxLegalMove> moves;
	const Position& pos;
public:
	MoveOrderer(const Position& pos, bool quiescence):pos(pos){
		n_moves = pos.generate_important_moves(moves, 0);
		if(!quiescence)n_moves = pos.generate_unimportant_moves(moves, n_moves);
	}
	Move next(){
		while(true){
			if(idx >= n_moves)return NullMove;
			if(pos.is_suicide_move(moves[idx])){
				idx++;continue;
			}
			return moves[idx++];
		}
	}
};

class Searcher{
	int search(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int qsearch(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int search_w(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	std::thread main_thread;
	bool stop_recieved;
public:
	void stop(){stop_recieved = true;}
	void go(State& state);
	int think(State& state, int max_depth, PV& pv, bool print);
	int think(State& state, int max_depth, bool print);
};
