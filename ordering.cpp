#include "search.hpp"

static float mvv_lva(Move m){
	return (material_value[m.capture()] << 8) - material_value[m.piece()];
}

MoveOrderer::MoveOrderer(const Position& pos, Move hash_move, const KillerMove& killer, bool do_fp)
	:idx(0), pos(pos), hash_move(hash_move), killer(killer), do_fp(do_fp){	
	status = Hash;
	if(hash_move != NullMove && pos.is_valid_move(hash_move)){
		moves[0] = hash_move;
		n_moves = 1;
	}
	else{
		n_moves = 0;
	}
}
MoveOrderer::MoveOrderer(const Position& pos, Move hash_move):idx(0), pos(pos), hash_move(hash_move){
	status = All;
	n_moves = pos.generate_important_moves(moves, 0);
	for(int i=0;i<n_moves;i++){
		if(pos.is_suicide_move(moves[i]))moves[i--] = moves[--n_moves];
		scores[i] = mvv_lva(moves[i]);
	}
	insertion_sort(0, n_moves);
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
				n_moves = pos.generate_important_moves(moves, n_moves);
				//killer
				if(!pos.check()){
					if(pos.is_valid_move(killer[0]))moves[n_moves++] = killer[0];
					if(pos.is_valid_move(killer[1]))moves[n_moves++] = killer[1];
				}
				for(int i=idx;i<n_moves;i++){
					if(moves[i] == hash_move || pos.is_suicide_move(moves[i]))moves[i--] = moves[--n_moves];
					else scores[i] = mvv_lva(moves[i]);
				}
				insertion_sort(idx, n_moves);
				status = Important;
				if(n_moves > idx)break;
			case Important:
				n_moves = pos.generate_unimportant_moves(moves, n_moves);
				for(int i=idx;i<n_moves;i++){
					Move m = moves[i];
					if(m == hash_move || m == killer[0] || m == killer[1]
						|| pos.is_suicide_move(moves[i]))moves[i--] = moves[--n_moves];
					//futility pruning
					//By pruning here, move scoring is omitted.
					else if(do_fp && !pos.is_move_check(moves[i]))moves[i--] = moves[--n_moves];
					else{
					}
				}
				status = All;
				if(n_moves > idx)break;
			case All:
				return NullMove;
			}
		}
		return moves[idx++];
	}
}
