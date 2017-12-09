#pragma once

#include "position.hpp"
#include "state.hpp"

using PV = Array<Move, 128>;

class MoveOrderer{
	int n_moves;
	int idx;
	Array<Move, MaxLegalMove> moves;
	Array<float, MaxLegalMove> scores;
	const Position& pos;
static float mvv_lva(Move m){
	return (material_value[m.capture()] << 8) - material_value[m.piece()];
}
public:
	MoveOrderer(const Position& pos, bool quiescence):idx(0), pos(pos){
		n_moves = pos.generate_important_moves(moves, 0);
		if(!quiescence)n_moves = pos.generate_unimportant_moves(moves, n_moves);
		if(quiescence){
			for(int i=0;i<n_moves;i++)scores[i] = mvv_lva(moves[i]);
			insertion_sort(0, n_moves);
		}
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
	void insertion_sort(int start, int end);
};

class Searcher{
	int search(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int qsearch(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int search_w(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	std::thread main_thread;
	bool stop_recieved;
	uint64_t nodes;
public:
	~Searcher(){
		if(main_thread.joinable())main_thread.join();
	}
	void stop(){
		stop_recieved = true;
		if(main_thread.joinable())main_thread.join();
	}
	void go(State& state);
	int think(State& state, int max_depth, PV& pv, bool print);
	int think(State& state, int max_depth, bool print);
};
