#include "position.hpp"
#include "bitboard.hpp"

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

int Position::evaluate(int rnd)const{
	const Player enemy = opponent(turn);
	//endgame knowledge
	if(!more_than_one(occupied[turn])){
		//K  vs  K + (R | Q) + alpha
		if(pieces[Queen] | pieces[Rook]){
			return -AlmostWin - sweep_table[king_sq[turn]];
		}
	}
	else if(!more_than_one(occupied[enemy])){
		//K + (R | Q) + alpha
		if(pieces[Queen] | pieces[Rook]){
			return AlmostWin + sweep_table[king_sq[enemy]];
		}
	}
	int n = popcnt(all_bb);
	if(n == 4 && more_than_one(pieces[Knight])){
		//K + N + N vs K
		//or
		//K + N vs K + N
		return 0;
	}
	//material value
	int v = rnd;
	for(Piece p = Pawn; p != King; p++){
		v += (popcnt(get_bb(turn, p)) - popcnt(get_bb(enemy, p))) * material_value[p];
	}
	return v;
}

