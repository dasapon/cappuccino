#include "search.hpp"

static float mvv_lva(Move m){
	return (material_value[m.capture()] << 8) - material_value[m.piece()];
}

MoveOrderer::MoveOrderer(const Position& pos, Move hash_move, bool quiescence):idx(0), pos(pos), hash_move(hash_move){
	if(!quiescence){
		status = Hash;
		if(hash_move != NullMove && pos.is_valid_move(hash_move)){
			moves[0] = hash_move;
			n_moves = 1;
		}
		else{
			n_moves = 0;
		}
	}
	else{
		status = All;
		n_moves = pos.generate_important_moves(moves, 0);
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
		if(idx >= n_moves){
			switch(status){
			case Hash:
				//generate moves
				n_moves = pos.generate_important_moves(moves, n_moves);
				n_moves = pos.generate_unimportant_moves(moves, n_moves);
				for(int i=idx;i<n_moves;i++){
					if(moves[i] == hash_move)moves[i--] = moves[--n_moves];
				}
				status = All;
				if(n_moves > idx)break;
			case All:
				return NullMove;
			}
		}
		if(pos.is_suicide_move(moves[idx])){
			idx++;
			continue;
		}
		return moves[idx++];
	}
}
