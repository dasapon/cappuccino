#include "state.hpp"
#include "search.hpp"

int Searcher::think(State& state, int max_depth, bool print){
	PV pv;
	return think(state, max_depth, pv, print);
}

int Searcher::think(State& state, int max_depth, PV& pv, bool print){
	int ret = 0;
	nodes = 0;
	hash_table.new_gen();
	for(int depth = 1; depth <= max_depth; depth++){
		ret = search(state, -MateValue, MateValue, depth, 0, pv);
		//print info
		if(print){
			std::cout << "info depth " << depth;
			std::cout << " time 0";
			std::cout << " nodes " << nodes;
			std::cout << " score cp " << ret;
			if(pv[0] != NullMove){
				std::cout << " pv";
				for(int i = 0;pv[i] != NullMove;i++){
					std::cout << " " << pv[i].to_fen();
				}
			}
			std::cout << std::endl;
		}
		if(std::abs(ret) >= MateValue || stop_recieved)break;
	}
	//bestmove
	if(print){
		if(pv[0] != NullMove){
			std::cout << "bestmove " << pv[0].to_fen() << std::endl;
		}
		else{
			std::cout << "bestmove resign"<<std::endl;
		}
	}
	return ret;
}

void Searcher::go(State& state){
	stop();
	stop_recieved = false;
	main_thread = std::thread([&](){
		think(std::ref(state), 10, true);
	});
}

int Searcher::search_w(State& state, int alpha, int beta, int depth, int ply, PV& pv_old){
	if(depth > 0)return search(state, alpha, beta, depth, ply, pv_old);
	else return qsearch(state, alpha, beta, depth, ply, pv_old);
}
int Searcher::search(State& state, int alpha, int beta, int depth, int ply, PV& pv_old){
	pv_old[ply] = NullMove;
	const int old_alpha = alpha;
	const Position& pos = state.pos();
	const bool check = pos.check();
	PV pv;
	int best_value = -MateValue;
	Move hash_move = NullMove;
	HashEntry hash_entry;
	Move best_move = NullMove;
	bool legal_move_exist = false;
	//is draw?
	if(state.draw() && ply > 0)return 0;
	//probe hash table
	if(hash_table.probe(pos, hash_entry)){
		hash_move = hash_entry.move();
		int hash_value;
		if(hash_entry.hash_cut(hash_value, alpha, beta, depth))return hash_value;
	}
	//generate moves
	MoveOrderer move_orderer(pos, hash_move, false);
	while(true){
		Move move = move_orderer.next();
		if(move == NullMove)break;
		state.make_move(move);
		nodes++;
		int v;
		if(!legal_move_exist){
			v = -search_w(state, -beta, -alpha, depth - 1, ply + 1, pv);
		}
		else {
			v = -search_w(state, -alpha-1, -alpha, depth - 1, ply + 1, pv);
			if(alpha < v && v < beta){
				v = -search_w(state, -beta, -alpha, depth - 1, ply + 1, pv);
			}
		}
		state.unmake_move();
		legal_move_exist = true;
		if(v > best_value){
			best_value = v;
			best_move = move;
			//update pv
			pv_old = pv;
			pv_old[ply] = move;
			//update alpha
			alpha = std::max(v, alpha);
			//beta cutoff
			if(v >= beta)break;
		}
	}
	//stalemate
	if(!legal_move_exist && !check)best_value = 0;
	hash_table.store(pos, best_move, depth, best_value, old_alpha, beta);
	return best_value;
}
int Searcher::qsearch(State& state, int alpha, int beta, int depth, int ply, PV& pv_old){
	pv_old[ply] = NullMove;
	//const int old_alpha = alpha;
	const Position& pos = state.pos();
	const bool check = pos.check();
	PV pv;
	int best_value = -MateValue;
	Move best_move = NullMove;
	//is draw?
	if(state.draw())return 0;
	//stand pat
	int stand_pat = pos.evaluate();
	if(!check){
		best_value = stand_pat;
		alpha = std::max(alpha, stand_pat);
		if(stand_pat >= beta)return stand_pat;
	}
	//generate moves
	MoveOrderer move_orderer(pos, NullMove, true);
	while(true){
		Move move = move_orderer.next();
		if(move == NullMove)break;
		state.make_move(move);
		nodes++;
		int v = -search_w(state, -beta, -alpha, depth - 1, ply + 1, pv);
		state.unmake_move();
		if(v > best_value){
			best_value = v;
			best_move = move;
			//update pv
			pv_old = pv;
			pv_old[ply] = move;
			//update alpha
			alpha = std::max(v, alpha);
			//beta cutoff
			if(v >= beta)break;
		}
	}
	return best_value;
}
