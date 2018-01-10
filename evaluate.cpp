#include "bitboard.hpp"
#include "state.hpp"
#include "learn.hpp"
#include "search.hpp"

constexpr int eval_scale = 256;

const Array<int, NSquare> sweep_table({
	4, 4, 4, 4, 4, 4, 4, 4,
	4, 3, 3, 3, 3, 3, 3, 4,
	4, 3, 2, 2, 2, 2, 3, 4,
	4, 3, 2, 1, 1, 2, 3, 4,
	4, 3, 2, 1, 1, 2, 3, 4,
	4, 3, 2, 2, 2, 2, 3, 4,
	4, 3, 3, 3, 3, 3, 3, 4,
	4, 4, 4, 4, 4, 4, 4, 4,
});

const Array<int, PieceDim> material_value({
	0,
	PawnValue,
	KnightValue,
	BishopValue,
	RookValue,
	QueenValue,
	MateValue,
});

static Array<int, piece_index_dim> weights;

int evaluate(const State& state){
	const Position& pos = state.pos();
	if(pos.check())return 0;
	int endgame = pos.endgame_eval();
	if(endgame != ValueINF)return endgame;
	int n_pieces;
	const Array<int, 32>& list = state.get_piece_list(&n_pieces);
	int v = 0;
	for(int i=0;i<n_pieces;i++){
		v += weights[list[i]];
	}
	return v / eval_scale;
}

int Position::endgame_eval()const{
	const Player enemy = opponent(turn);
	int n = popcnt(all_bb);
	//endgame knowledge
	if(!more_than_one(occupied[turn])){
		//K  vs  K + (R | Q) + alpha
		if(pieces[Queen] | pieces[Rook]){
			return -AlmostWin - sweep_table[king_sq[turn]];
		}
		//K+N+N vs K
		if(n == 4 && more_than_one(pieces[Knight])){
			return 0;
		}
	}
	else if(!more_than_one(occupied[enemy])){
		//K + (R | Q) + alpha
		if(pieces[Queen] | pieces[Rook]){
			return AlmostWin + sweep_table[king_sq[enemy]];
		}
		//K+N+N vs K
		if(n == 4 && more_than_one(pieces[Knight])){
			return 0;
		}
	}
	else if(n == 4 && more_than_one(pieces[Knight] | pieces[Bishop])){
		//K+(N|B) vs K+(N|B)
		return 0;
	}
	return ValueINF;
}

void load_eval(){
	FILE * fp = fopen("eval.bin", "rb");
	bool fail = true;
	if(fp != NULL){
		fail = fread(&weights, sizeof(Array<int, piece_index_dim>), 1, fp) != 1;
		fclose(fp);
	}
	if(fail){
		std::cout << "eval loading is failed." << std::endl;
	}

}

#ifdef LEARN

static void eval_feature(State& state, const PV& pv, std::vector<int>& grad, int d){
	int ply = 0;
	for(;pv[ply] != NullMove;ply++){
		state.make_move(pv[ply]);
	}
	do{
		if(state.draw())break;
		const Position& pos = state.pos();
		if(pos.check())break;
		if(pos.endgame_eval() != ValueINF)break;
		d = pos.turn_player() == White? d : -d;
		int n_pieces;
		const Array<int, 32>& list = state.get_piece_list(&n_pieces);
		for(int i=0;i < n_pieces;i++){
			grad[list[i]] += d;
		}
	}while(false);
	for(int i=0;i<ply;i++){
		state.unmake_move();
	}
}

static bool learn_one(Searcher& searcher, State& state, Move bestmove, std::mt19937& mt, bool* mated){
	PV good_pv, bad_pv;
	//search subtree of bestmove
	state.make_move(bestmove);
	int good_v = -searcher.think(state, 2, good_pv, false);
	state.unmake_move();
	if(std::abs(good_v) >= MateValue){
		*mated = true;
		return true;
	}
	std::vector<int> grad(piece_index_dim, 0);
	//search subtree of badmoves
	//move generation
	Array<Move, MaxLegalMove> moves;
	int n_moves;
	{
		const Position& pos = state.pos();
		n_moves = pos.generate_important_moves(moves, 0);
		n_moves = pos.generate_unimportant_moves(moves, n_moves);
		for(int i=0;i<n_moves;i++){
			if(pos.is_suicide_move(moves[i]) || moves[i] == bestmove)moves[i--] = moves[--n_moves];
		}
	}
	int selected = 0;
	int cnt = 0;
	const int grad_scale = state.pos().turn_player() == White? eval_scale : -eval_scale;
	for(int i=0;i<n_moves;i++){
		if(mt() % (n_moves - i) >= 16 - selected)continue;
		selected++;
		state.make_move(moves[i]);
		int bad_v = -searcher.think(state, 2, bad_pv, false);
		if(bad_v >= good_v && std::abs(bad_v) < MateValue){
			cnt++;
			eval_feature(state, bad_pv, grad, -grad_scale);
		}
		state.unmake_move();
	}
	if(cnt > 0){
		state.make_move(bestmove);
		eval_feature(state, good_pv, grad, grad_scale * cnt);
		state.unmake_move();
		//update params
		for(int i=0;i<weights.size();i++){
			weights[i] += grad[i] / cnt;
		}
	}
	return cnt == 0;
}

void learn_eval(std::vector<Record>& records){
	//init evaluation table
	for(Piece p = Pawn; p != PieceDim; p++){
		for(Square sq = 0; sq < NSquare; sq++){
			if(p == Pawn && (sq < 8 || sq >=56))continue;
			weights[piece_index<false>(p, sq, White)] = material_value[p] * eval_scale;
			weights[piece_index<true>(p, sq, White)] = -material_value[p] * eval_scale;
		}
	}
	if(records.size() < 10000){
		std::cout << "Data set is too small !" << std::endl;
		return;
	}
	std::cout << "learning start" << std::endl;
	std::vector<Sample> training_set = get_training_set(records, 0);
	std::mt19937 mt(0);
	//training
	std::vector<int64_t> weights_sum(piece_index_dim, 0);
	int hit_count = 0;
	int mated_count = 0;
	Searcher searcher;
	State state;
	Timer timer;
	std::shuffle(training_set.begin(), training_set.end(), mt);
	int cnt = 0;
	for(int i=0;i < training_set.size();i++){
		if(i % 10000 == 0){
			std::cout << hit_count << " / " << i << " " <<mated_count << " " << timer.sec() << "[sec]" << std::endl;
			int64_t min = 0;
			for(int j=0;j<weights.size(); j++){
				min = std::min(min, weights_sum[j]);
			}
			std::cout << min << std::endl;
		}
		Sample sample = training_set[i];
		setup_leaning_position(state, records, sample);
		//Online MMTO
		bool mated = false;
		bool ok = learn_one(searcher, state, records[sample.first][sample.second], mt, &mated);
		if(ok)hit_count++;
		if(mated)mated_count++;
		for(int j=0;j<weights.size(); j++){
			weights_sum[j] += weights[j];
		}
		cnt++;
	}
	if(cnt != training_set.size())std::cout << "Invalid" << std::endl;
	//averaging
	for(int i=0;i<weights.size(); i++){
		weights[i] = weights_sum[i] / cnt;
	}
	FILE* fp = fopen("eval.bin", "wb");
	fwrite(&weights, sizeof(Array<int, piece_index_dim>), 1, fp);
	fclose(fp);
}

#endif
