#pragma once

#include "position.hpp"
#include "state.hpp"
#include "hash_table.hpp"

using PV = Array<Move, 128>;

class MoveOrderer{
	enum Status{
		Hash,
		All,
	};
	Status status;
	int n_moves;
	int idx;
	Array<Move, MaxLegalMove> moves;
	Array<float, MaxLegalMove> scores;
	const Position& pos;
	Move hash_move;
	void insertion_sort(int start, int end);
	
public:
	MoveOrderer(const Position& pos, Move hash_move, bool quiescence);
	Move next();
};

class Searcher{
	int search(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int qsearch(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	int search_w(State& state, int alpha, int beta, int depth, int ply, PV& pv_old);
	std::thread main_thread;
	bool stop_recieved;
	uint64_t nodes;
	HashTable hash_table;
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
