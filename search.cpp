#include "state.hpp"
#include "search.hpp"
#include "evaluate.hpp"

static int futility_margin(int depth){
	if(depth > 2 * depth_scale)return MateValue * 2;
	else return KnightValue;
}
static constexpr int delta_margin = PawnValue * 3;

int Searcher::think(State& state, int max_depth, bool print){
	PV pv;
	return think(state, max_depth, pv, print);
}

int Searcher::think(State& state, int max_depth, PV& pv, bool print){
	return think(state, max_depth, pv, print, false);
}
int Searcher::think_with_timer(State& state, int max_depth, bool print){
	PV pv;
	return think(state, max_depth, pv, print, true);
}
int Searcher::think(State& state, int max_depth, PV& pv, bool print, bool wait_timer_stopping){
	int ret = 0;
	nodes = 0;
	Timer timer;
	hash_table.new_gen();
	killer[0].clear();
	killer[1].clear();
	pv_table[0][0] = NullMove;
	//show move probability
	if(print){
		MoveOrdering move_ordering(state, NullMove, killer[0], true, false);
		std::cout << "info string moves";
		for(int i=0;i<4;i++){
			float score;
			Move move = move_ordering.next(&score);
			if(move == NullMove)break;
			std::cout << "(" << move.to_lan() <<", "<< score << "), ";
		}
		std::cout << std::endl;
	}
	for(int depth = 1; depth <= max_depth; depth++){
		int v = search(state, -MateValue, MateValue, depth * depth_scale, 0);
		if(v > INT_MIN)ret = v;
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
		if(std::abs(ret) >= MateValue || abort)break;
	}
	if(wait_timer_stopping){
		while(!abort){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	//bestmove
	if(print){
		if(pv[0] != NullMove){
			std::cout << "bestmove " << pv[0].to_lan();
			if(pv[1] != NullMove){
				std::cout << " ponder " << pv[1].to_lan();
			}
			std::cout << std::endl;
		}
		else{
			std::cout << "bestmove resign"<<std::endl;
		}
	}
	return ret;
}

void Searcher::go(State& state, uint64_t time, uint64_t inc, bool ponder_or_infinite){
	stop();
	abort = false;
	timer_start(time, inc, ponder_or_infinite);
	main_thread = std::thread([&](){
		think_with_timer(std::ref(state), 32, true);
	});
}

int Searcher::search_w(State& state, int alpha, int beta, int depth, int ply){
	if(depth >= depth_scale)return search(state, alpha, beta, depth, ply);
	else return qsearch(state, alpha, beta, 0, ply);
}
int Searcher::search(State& state, int alpha, int beta, int depth, int ply){
	const int old_alpha = alpha;
	const Position& pos = state.pos();
	const bool check = pos.check();
	int best_value = INT_MIN;
	Move hash_move = NullMove;
	HashEntry hash_entry;
	Move best_move = NullMove;
	bool is_pv = beta - alpha > 1;
	if(ply == 0)hash_move = pv_table[ply][ply];
	else pv_table[ply][ply] = NullMove;
	//is draw?
	if(state.draw() && ply > 0)return 0;
	//probe hash table
	if(ply > 0 && hash_table.probe(pos, hash_entry)){
		hash_move = hash_entry.move();
#ifndef LEARN
		int hash_value;
		if(hash_entry.hash_cut(hash_value, alpha, beta, depth)){
			pv_table[ply][ply] = hash_move;
			pv_table[ply][ply + 1] = NullMove;
			return hash_value;
		}
#endif
	}
	killer[ply + 2].clear();
	int stand_pat = evaluate(state);
	int fut_margin = futility_margin(depth);
	const bool do_fp = !is_pv && !check && stand_pat + fut_margin < alpha;
	if(do_fp)best_value = std::max(best_value, stand_pat + fut_margin);
	//generate moves
	int move_count = 0;
	const bool rps = depth >= RPSDepth;
	MoveOrdering move_ordering(state, hash_move, killer[ply], rps, do_fp);
	while(true){
		float score;
		Move move = move_ordering.next(&score);
		if(move == NullMove)break;
		state.make_move(move);
		nodes++;
		move_count++;
		int consume = depth_scale;
		if(rps){
			//realization probability search
			consume = -std::log2(score) * depth_scale;
			consume = std::min(consume, depth - 2 * depth_scale);
			if(move_count == 1)consume = std::min(consume, depth_scale);
		}
		else{
			//check extention
			if(pos.is_move_check(move))consume = depth_scale / 2;
			else if(depth >= 2 * depth_scale
					&& !check
					&& move_count > 8
					&& !move.is_important()
					&& move != killer[ply][0]
					&& move != killer[ply][1]){
				//late move reduction
				consume = 2 * depth_scale;
			}
		}
		int v;
		if(move_count == 1){
			v = -search_w(state, -beta, -alpha, depth - depth_scale, ply + 1);
		}
		else {
			v = -search_w(state, -alpha-1, -alpha, depth - consume, ply + 1);
			if(consume > depth_scale && v > alpha){
				consume = depth_scale;
				v = -search_w(state, -alpha-1, -alpha, depth - consume, ply + 1);
			}
			if(alpha < v && v < beta){
				v = -search_w(state, -beta, -alpha, depth - consume, ply + 1);
			}
		}
		state.unmake_move();
		if(abort)return best_value;
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
	if(move_count == 0 && !check && !do_fp)best_value = 0;
	//checkmate
	else best_value = std::max(best_value, -MateValue);
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
#ifndef LEARN
		int hash_value;
		if(ply > 0 && hash_entry.hash_cut(hash_value, alpha, beta, depth)){
			pv_table[ply][ply] = hash_move;
			pv_table[ply][ply + 1] = NullMove;
			return hash_value;
		}
#endif
	}
	//stand pat
	int stand_pat = evaluate(state);
	if(!check){
		best_value = stand_pat;
		alpha = std::max(alpha, stand_pat);
		if(stand_pat >= beta)return stand_pat;
	}
	//generate moves
	MoveOrdering move_ordering(state, hash_move);
	while(true){
		float score;
		Move move = move_ordering.next(&score);
		if(move == NullMove)break;
		//pruning
		if(!check && !pos.is_move_check(move)){
			if(pos.see(move) < 0)continue;
			//delta pruning
			int max_gain = delta_margin + material_value[move.capture()];
			if(move.is_promotion())max_gain += material_value[move.piece_moved()] - PawnValue; 
			if(stand_pat + max_gain <= alpha){
				best_value = std::max(stand_pat + max_gain, best_value);
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

void Searcher::timer_start(uint64_t t, uint64_t i, bool ponder_or_infinite){
	inf = ponder_or_infinite;
	uint64_t opt = t / 24;
	Timer timer;
	search_start = timer;
	timer_thread = std::thread([&](uint64_t time, uint64_t inc, uint64_t optimum){
		while(true){
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			if(!inf && search_start.msec() > optimum)break;
		}
		abort = true;
	}, t, i, opt);
}
