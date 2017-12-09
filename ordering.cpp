#include "search.hpp"

static float mvv_lva(Move m){
	return (material_value[m.capture()] << 8) - material_value[m.piece()];
}

MoveOrderer::MoveOrderer(const Position& pos, bool quiescence):idx(0), pos(pos){
	n_moves = pos.generate_important_moves(moves, 0);
	if(!quiescence)n_moves = pos.generate_unimportant_moves(moves, n_moves);
	if(quiescence){
		for(int i=0;i<n_moves;i++)scores[i] = mvv_lva(moves[i]);
		insertion_sort(0, n_moves);
	}
}
void MoveOrderer::insertion_sort(int start, int end){
	int i, j;
	float s;
	Move m;
	for (i = start + 1; i < end; i++) {
		s = scores[i];
		m = moves[i];
		if (s > scores[i - 1]) {
			j = i;
			do {
				scores[j] = scores[j - 1];
				moves[j] = moves[j - 1];
				j--;
			} while (j  > start && s > scores[j - 1]);
			scores[j] = s;
			moves[j] = m;
		}
	}
}

Move MoveOrderer::next(){
	while(true){
		if(idx >= n_moves)return NullMove;
		if(pos.is_suicide_move(moves[idx])){
			idx++;
			continue;
		}
		return moves[idx++];
	}
}
