#include "bitboard.hpp"
#include "state.hpp"
#include "learn.hpp"
#include "search.hpp"

constexpr int eval_scale = 256;

const sheena::Array<int, NSquare> sweep_table({
	4, 4, 4, 4, 4, 4, 4, 4,
	4, 3, 3, 3, 3, 3, 3, 4,
	4, 3, 2, 2, 2, 2, 3, 4,
	4, 3, 2, 1, 1, 2, 3, 4,
	4, 3, 2, 1, 1, 2, 3, 4,
	4, 3, 2, 2, 2, 2, 3, 4,
	4, 3, 3, 3, 3, 3, 3, 4,
	4, 4, 4, 4, 4, 4, 4, 4,
});

const sheena::Array<int, PieceDim> material_value({
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
	sheena::Array2d<Ty, piece_list_dim, piece_list_dim> piece_pair;
	template<typename T2>
	void operator+=(const EvalTable<T2>& tbl){
		for(int i=0;i<piece_list_dim;i++){
			for(int j=0;j<piece_list_dim;j++){
				piece_pair[i][j] += tbl.piece_pair[i][j];
			}
		}
	}
	void operator/=(int x){
		for(int i=0;i<piece_list_dim;i++){
			for(int j=0;j<piece_list_dim;j++){
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
	for(int i=0;i<piece_list_dim;i++){
		for(int j=0;j<piece_list_dim;j++){
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
	const PieceList& pl = state.piece_list();
	int v = 0;
	//piece pair
	for(int i=0;i<pl.size;i++){
		const int p1 =pl.list[i].index<White>();
		for(int j=i;j<pl.size;j++){
			v += weights.piece_pair[p1][pl.list[j].index<White>()];
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
