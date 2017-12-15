#pragma once

#include "position.hpp"
#include "state.hpp"
#include "hash_table.hpp"

constexpr int max_ply = 128;

using PV = Array<Move, max_ply>;
class KillerMove : public Array<Move, 2>{
	public:
	void update(Move m){
		if((*this)[0] != m){
			(*this)[1] = (*this)[0];
			(*this)[0] = m;
		}
	}
	void clear(){
		(*this)[1] = (*this)[0] = NullMove;
	}
};

class MoveOrderer{
	enum Status{
		Hash,
		Important,
		All,
	};
	Status status;
	int n_moves;
	int idx;
	Array<Move, MaxLegalMove> moves;
	Array<float, MaxLegalMove> scores;
	const State& state;
	Move hash_move;
	KillerMove killer;
	bool do_fp;
	void insertion_sort(int start, int end);
	
public:
	MoveOrderer(const State& state, Move hash_move, const KillerMove& killer, bool do_fp);
	MoveOrderer(const State& state, Move hash_move);
	Move next();
};

class Searcher{
	int search(State& state, int alpha, int beta, int depth, int ply);
	int qsearch(State& state, int alpha, int beta, int depth, int ply);
	int search_w(State& state, int alpha, int beta, int depth, int ply);
	std::thread main_thread;
	bool stop_recieved;
	uint64_t nodes;
	HashTable hash_table;
	int evaluate(const Position& pos){
		return pos.evaluate();
	}
	PV pv_table[max_ply];
	KillerMove killer[max_ply + 2];
public:
	~Searcher(){
		if(main_thread.joinable())main_thread.join();
	}
	Searcher(){
	}
	void stop(){
		stop_recieved = true;
		if(main_thread.joinable())main_thread.join();
	}
	void go(State& state);
	int think(State& state, int max_depth, PV& pv, bool print);
	int think(State& state, int max_depth, bool print);
};
