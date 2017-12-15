#include "state.hpp"
#include "search.hpp"
#include "time.hpp"

int Searcher::think(State& state, int max_depth, bool print){
	PV pv;
	return think(state, max_depth, pv, print);
}
int futility_margin(int depth){
	if(depth > 2 * depth_scale)return MateValue * 2;
	else return KnightValue;
}
constexpr int delta_margin = KnightValue;
int Searcher::think(State& state, int max_depth, PV& pv, bool print){
	int ret = 0;
	nodes = 0;
	Timer timer;
	hash_table.new_gen();
	killer[0].clear();
	killer[1].clear();
	for(int depth = 1; depth <= max_depth; depth++){
		ret = search(state, -MateValue, MateValue, depth * depth_scale, 0);
		pv = pv_table[0];
		//print info
		if(print){
			std::cout << "info depth " << depth;
			std::cout << " time " << timer.msec();
			std::cout << " nodes " << nodes;
			std::cout << " score cp " << ret;
			if(pv[0] != NullMove){
				std::cout << " pv";
				for(int i = 0;pv[i] != NullMove;i++){
					std::cout << " " << pv[i].to_lan();
				}
			}
			std::cout << std::endl;
		}
		if(std::abs(ret) >= MateValue || stop_recieved)break;
	}
	//bestmove
	if(print){
		if(pv[0] != NullMove){
			std::cout << "bestmove " << pv[0].to_lan() << std::endl;
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
		think(std::ref(state), 19, true);
	});
}

int Searcher::search_w(State& state, int alpha, int beta, int depth, int ply){
	if(depth > 0)return search(state, alpha, beta, depth, ply);
	else return qsearch(state, alpha, beta, depth, ply);
}
int Searcher::search(State& state, int alpha, int beta, int depth, int ply){
	pv_table[ply][ply] = NullMove;
	const int old_alpha = alpha;
	const Position& pos = state.pos();
	const bool check = pos.check();
	int best_value = -MateValue;
	Move hash_move = NullMove;
	HashEntry hash_entry;
	Move best_move = NullMove;
	bool legal_move_exist = false;
	bool is_pv = beta - alpha > 1;
	//is draw?
	if(state.draw() && ply > 0)return 0;
	//probe hash table
	if(hash_table.probe(pos, hash_entry)){
		hash_move = hash_entry.move();
		int hash_value;
		if(ply > 0 && hash_entry.hash_cut(hash_value, alpha, beta, depth)){
			pv_table[ply][ply] = hash_move;
			pv_table[ply][ply + 1] = NullMove;
			return hash_value;
		}
	}
	killer[ply + 2].clear();
	int stand_pat = evaluate(pos);
	int fut_margin = futility_margin(depth);
	const bool do_fp = !is_pv && !check && stand_pat + fut_margin < alpha;
	if(do_fp)best_value = std::max(best_value, stand_pat + fut_margin);
	//generate moves
	MoveOrderer move_orderer(state, hash_move, killer[ply], do_fp);
	while(true){
		Move move = move_orderer.next();
		if(move == NullMove)break;
		state.make_move(move);
		nodes++;
		int v;
		if(!legal_move_exist){
			v = -search_w(state, -beta, -alpha, depth - depth_scale, ply + 1);
		}
		else {
			v = -search_w(state, -alpha-1, -alpha, depth - depth_scale, ply + 1);
			if(alpha < v && v < beta){
				v = -search_w(state, -beta, -alpha, depth - depth_scale, ply + 1);
			}
		}
		state.unmake_move();
		legal_move_exist = true;
		if(v > best_value){
			best_value = v;
			best_move = move;
			//update pv
			pv_table[ply] = pv_table[ply + 1];
			pv_table[ply][ply] = move;
			//update alpha
			alpha = std::max(v, alpha);
			//beta cutoff
			if(v >= beta)break;
		}
	}
	if(best_value > old_alpha
		&& !check
		&& !best_move.is_important()){
		killer[ply].update(best_move);
	}
	//stalemate
	if(!legal_move_exist && !check)best_value = 0;
	hash_table.store(pos, best_move, depth, best_value, old_alpha, beta);
	return best_value;
}
int Searcher::qsearch(State& state, int alpha, int beta, int depth, int ply){
	pv_table[ply][ply] = NullMove;
	const int old_alpha = alpha;
	const Position& pos = state.pos();
	const bool check = pos.check();
	int best_value = -MateValue;
	Move best_move = NullMove;
	Move hash_move = NullMove;
	HashEntry hash_entry;
	//is draw?
	if(state.draw())return 0;
	//probe hash table
	if(hash_table.probe(pos, hash_entry)){
		hash_move = hash_entry.move();
		int hash_value;
		if(ply > 0 && hash_entry.hash_cut(hash_value, alpha, beta, depth)){
			pv_table[ply][ply] = hash_move;
			pv_table[ply][ply + 1] = NullMove;
			return hash_value;
		}
	}
	//stand pat
	int stand_pat = evaluate(pos);
	if(!check){
		best_value = stand_pat;
		alpha = std::max(alpha, stand_pat);
		if(stand_pat >= beta)return stand_pat;
	}
	//generate moves
	MoveOrderer move_orderer(state, NullMove);
	while(true){
		Move move = move_orderer.next();
		if(move == NullMove)break;
		//pruning
		if(!check && !pos.is_move_check(move)){
			if(pos.see(move) < 0)continue;
			if(stand_pat + delta_margin <= alpha){
				best_value = std::max(stand_pat + delta_margin, best_value);
				continue;
			}
		}
		state.make_move(move);
		nodes++;
		int v = -search_w(state, -beta, -alpha, depth - depth_scale, ply + 1);
		state.unmake_move();
		if(v > best_value){
			best_value = v;
			best_move = move;
			//update pv
			pv_table[ply] = pv_table[ply + 1];
			pv_table[ply][ply] = move;
			//update alpha
			alpha = std::max(v, alpha);
			//beta cutoff
			if(v >= beta)break;
		}
	}
	hash_table.store(pos, best_move, depth, best_value, old_alpha, beta);
	return best_value;
}

