#pragma once

#include "position.hpp"
#include "state.hpp"
#include "hash_table.hpp"
#include "time.hpp"

constexpr int max_ply = 128;

using PV = Array<Move, max_ply>;

enum {
	RPSDepth = 4 * depth_scale,
};
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

class MoveOrdering{
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
	MoveOrdering(const State& state, Move hash_move, const KillerMove& killer, bool rps, bool do_fp);
	MoveOrdering(const State& state, Move hash_move);
	Move next(float* score);
};

class Searcher{
	int search(State& state, int alpha, int beta, int depth, int ply);
	int qsearch(State& state, int alpha, int beta, int depth, int ply);
	int search_w(State& state, int alpha, int beta, int depth, int ply);
	std::thread main_thread, timer_thread;
	uint64_t nodes;
	HashTable hash_table;
	PV pv_table[max_ply];
	KillerMove killer[max_ply + 2];
public:
	~Searcher(){
		if(main_thread.joinable())main_thread.join();
	}
	Searcher(){}
	void go(State& state, uint64_t time, uint64_t inc, bool ponder_or_infinite);
	int think(State& state, int max_depth, PV& pv, bool print);
	int think(State& state, int max_depth, bool print);
	void hash_size(size_t mb){hash_table.set_size(mb);}
	uint64_t node_searched()const{return nodes;}
	//time control
private:
	volatile bool abort;
	volatile bool inf;
	Timer search_start;
	void timer_start(uint64_t, uint64_t, bool);
public:
	void ponder_hit(){
		inf = false;
	}
	void stop(){
		inf = false;
		abort = true;
		if(main_thread.joinable())main_thread.join();
		if(timer_thread.joinable())timer_thread.join();
	}
};
