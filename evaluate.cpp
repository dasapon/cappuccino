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

template<typename Ty>
struct EvalTable{
	Array<Array<Ty, piece_index_dim>, piece_index_dim> piece_pair;
	template<typename T2>
	void operator+=(const EvalTable<T2>& tbl){
		for(int i=0;i<piece_index_dim;i++){
			for(int j=0;j<piece_index_dim;j++){
				piece_pair[i][j] += tbl.piece_pair[i][j];
			}
		}
	}
	void operator/=(int x){
		for(int i=0;i<piece_index_dim;i++){
			for(int j=0;j<piece_index_dim;j++){
				piece_pair[i][j] /= x;
			}
		}
	}
	void clear(){
		memset(this, 0, sizeof(EvalTable<Ty>));
	}
};
template<typename F, typename T1, typename T2>
static void for_each_table(EvalTable<T1>& tbl1, EvalTable<T2>& tbl2, F proce){
	for(int i=0;i<piece_index_dim;i++){
		for(int j=0;j<piece_index_dim;j++){
			proce(tbl1.piece_pair[i][j], tbl2.piece_pair[i][j]);
		}
	}
}

static EvalTable<int> weights;

int evaluate(const State& state){
	const Position& pos = state.pos();
	if(pos.check())return 0;
	int endgame = pos.endgame_eval();
	if(endgame != ValueINF)return endgame;
	int n_pieces;
	const Array<int, 32>& list = state.get_piece_list(&n_pieces);
	int v = 0;
	//piece pair
	for(int i=0;i<n_pieces;i++){
		const int p1 =list[i];
		for(int j=i;j<n_pieces;j++){
			v += weights.piece_pair[p1][list[j]];
		}
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
		if(n == 4){
			//K+N+N vs K
			if(more_than_one(pieces[Knight])){
				return 0;
			}
			//K+B+B vs K
			if(more_than_one(pieces[Bishop])){
				return -AlmostWin - sweep_table[king_sq[turn]];
			}
		}
	}
	else if(!more_than_one(occupied[enemy])){
		//K + (R | Q) + alpha
		if(pieces[Queen] | pieces[Rook]){
			return AlmostWin + sweep_table[king_sq[enemy]];
		}
		if(n == 4){
			//K+N+N vs K
			if(more_than_one(pieces[Knight])){
				return 0;
			}
			//K+B+B vs K
			if(more_than_one(pieces[Bishop])){
				return AlmostWin + sweep_table[king_sq[turn]];
			}
		}
	}

	else if(n == 4){
		//K+(N|B) vs K+(N|B)
		if(more_than_one(pieces[Knight] | pieces[Bishop])){
			return 0;
		}
		//K+B vs K+P
		if(pieces[Bishop] != 0 && pieces[Pawn] != 0){
			return 0;
		}
		//K+R vs K+R
		if(more_than_one(pieces[Rook]))return 0;
		//K+Q vs K+Q
		if(more_than_one(pieces[Queen]))return 0;
	}
	return ValueINF;
}

void load_eval(){
	FILE * fp = fopen("eval.bin", "rb");
	bool fail = true;
	if(fp != NULL){
		fail = fread(&weights, sizeof(EvalTable<int>), 1, fp) != 1;
		fclose(fp);
	}
	if(fail){
		std::cout << "eval loading is failed." << std::endl;
	}

}

#ifdef LEARN

static void eval_feature(State& state, const PV& pv, EvalTable<int>& grad, int d){
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
		//piece pair
		for(int i=0;i<n_pieces;i++){
			const int p1 =list[i];
			for(int j=i;j<n_pieces;j++){
				const int p2 = list[j];
				grad.piece_pair[p1][p2] += d;
				grad.piece_pair[p2][p1] += d;
			}
		}
	}while(false);
	for(int i=0;i<ply;i++){
		state.unmake_move();
	}
}

static bool learn_one(Searcher& searcher, State& state, Move bestmove, std::mt19937& mt, bool* mated,
	EvalTable<int>& grad){
	PV good_pv, bad_pv;
	//search subtree of bestmove
	state.make_move(bestmove);
	int good_v = -searcher.think(state, 2, good_pv, false);
	state.unmake_move();
	if(std::abs(good_v) >= MateValue){
		*mated = true;
		return true;
	}
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
	const int grad_scale = (state.pos().turn_player() == White? eval_scale : -eval_scale) / 8;
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
		for_each_table(weights, grad, [=](int& w, int& g){
			w += g / cnt;
			g = 0;
		});
	}
	return cnt == 0;
}

void learn_eval(std::vector<Record>& records){
	//init evaluation table
	weights.clear();
	for(Piece p = PassedPawn; p != PieceDim; p++){
		for(Square sq = 0; sq < NSquare; sq++){
			if((p == Pawn || p == PassedPawn) && (sq < 8 || sq >=56))continue;
			int id = piece_index<false>(p, sq, White);
			int id_rev = piece_index<true>(p, sq, White);
			int v = (p == PassedPawn? PawnValue : material_value[p]) * eval_scale;
			weights.piece_pair[id][id] = v;
			weights.piece_pair[id_rev][id_rev] = -v;
		}
	}
	std::cout << "learning start" << std::endl;
	std::vector<Sample> training_set = get_training_set(records, 0);
	std::mt19937 mt(0);
	//training
	std::unique_ptr<EvalTable<int64_t>> weights_sum(new EvalTable<int64_t>);
	std::unique_ptr<EvalTable<int>> grad(new EvalTable<int>);
	weights_sum->clear();
	grad->clear();
	int hit_count = 0;
	int mated_count = 0;
	Searcher searcher;
	State state;
	Timer timer;
	std::shuffle(training_set.begin(), training_set.end(), mt);
	int cnt = 0;
	int b = 0;
	for(int i=0;i < training_set.size();i++){
		if(i % 50000 == 0){
			std::cout << hit_count << " / " << i << " " << mated_count <<  " " << timer.sec() << "[sec]" << std::endl;
		}
		Sample sample = training_set[i];
		setup_leaning_position(state, records, sample);
		//Online MMTO
		bool mated = false;
		bool ok = learn_one(searcher, state, records[sample.first][sample.second], mt, &mated, *grad);
		if(ok)hit_count++;
		if(mated)mated_count++;
		b++;
		if(!ok || i + 1== training_set.size()){
			for_each_table(*weights_sum, weights, [=](int64_t& x, int y){
				x += b * y;
			});
			cnt+=b;
			b = 0;
		}
	}
	if(cnt != training_set.size())std::cout << "Invalid" << std::endl;
	//averaging
	(*weights_sum) /= cnt;
	weights.clear();
	weights += (*weights_sum);
	FILE* fp = fopen("eval.bin", "wb");
	fwrite(&weights, sizeof(EvalTable<int>), 1, fp);
	fclose(fp);
}

#endif
