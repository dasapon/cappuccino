#include "position.hpp"

uint64_t performance_test(Position& pos, int depth){
	if(depth == 0)return 1;
	Array<Move, MaxLegalMove> moves;
	int n = pos.generate_important_moves(moves, 0);
	n = pos.generate_unimportant_moves(moves, n);
	uint64_t ret = 0;
	for(int i=0;i<n;i++){
		if(pos.is_suicide_move(moves[i]))continue;
		if(moves[i].capture()!=Empty)ret++;
		Position next(pos);
		next.make_move(moves[i]);
		ret += performance_test(next, depth - 1);
	}
	return ret;
}
