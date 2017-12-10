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
	const Position& pos;
	Move hash_move;
	KillerMove killer;
	void insertion_sort(int start, int end);
	
public:
	MoveOrderer(const Position& pos, Move hash_move, const KillerMove& killer);
	MoveOrderer(const Position& pos, Move hash_move);
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
	//
	Array<int, 0x4000> random_table;
	int evaluate(const Position& pos){
		return pos.evaluate(random_table[pos.key() & 0x3fff]);
	}
	PV pv_table[max_ply];
	KillerMove killer[max_ply + 2];
public:
	void set_randomness(int sd);
	~Searcher(){
		if(main_thread.joinable())main_thread.join();
	}
	Searcher(){
		set_randomness(10);
	}
	void stop(){
		stop_recieved = true;
		if(main_thread.joinable())main_thread.join();
	}
	void go(State& state);
	int think(State& state, int max_depth, PV& pv, bool print);
	int think(State& state, int max_depth, bool print);
};
