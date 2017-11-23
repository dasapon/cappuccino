#include "bitboard.hpp"

BitBoardTable<64> knight_attack_table, king_attack_table;
BitBoardTable<64> rank_mask_table, file_mask_table, diag_mask_table, diag2_mask_table;

Array<BitBoardTable<64>, 8> rank_diag_attack_table, file_attack_table;

void init_tables(){
	const BitBoard a_file = 0x0101010101010101ULL;
	const BitBoard h_file = 0x8080808080808080ULL;
	const BitBoard first_rank = 0xffULL;
	
	auto left = [=](BitBoard bb){return (bb >> 1) & ~h_file;};
	auto right = [=](BitBoard bb){return (bb << 1) & ~a_file;};
	for(Square sq = 0;sq < NSquare;sq++){
		rank_mask_table[sq] = first_rank << (sq / 8);
		file_mask_table[sq] = a_file << (sq % 8);
		//init diag_mask and diag2_mask
		BitBoard diag1 = bb_sq(sq);
		while(true){
			BitBoard bb = ((diag1 << 9) & ~a_file) | ((diag1 >> 9) & ~h_file);
			if((bb | diag1) == diag1)break;
			diag1 |= bb;
		}
		diag_mask_table[sq] = diag1;
		BitBoard diag2 = bb_sq(sq);
		while(true){
			BitBoard bb = ((diag2 << 7) & ~h_file) | ((diag2 >> 7) & ~a_file);
			if((bb | diag2) == diag2)break;
			diag2 |= bb;
		}
		diag2_mask_table[sq] = diag2;
		
		//init knight_attack
		const BitBoard lr = left(bb_sq(sq)) | right(bb_sq(sq));
		const BitBoard ll_rr = left(left(bb_sq(sq))) | right(right(bb_sq(sq)));
		knight_attack_table[sq] = (lr << 16) | (lr >> 16) | (ll_rr << 8) | (ll_rr >> 8);
		
		//init king_attack
		king_attack_table[sq] = lr | ((lr | bb_sq(sq)) << 8) | ((lr | bb_sq(sq)) >> 8);
	}
	
	//init sliding_attack_table
	for(int i = 0; i < 8;i++){
		for(BitBoard occ = 0;occ < 64; occ++){
			const BitBoard occupied = occ << 1;
			BitBoard rd_attack = 0, f_attack = 0;
			//left
			for(int j = i - 1;j>=0;j--){
				rd_attack |= 1ULL << j;
				f_attack |= 1ULL << (j * 8);
				if(occupied & (1ULL << j))break;
			}
			//right
			for(int j = i + 1;j < 8;j++){
				rd_attack |= 1ULL << j;
				f_attack |= 1ULL << (j * 8);
				if(occupied & (1ULL << j))break;
			}
			for(int j=1;j<8;j++){
				rd_attack |= rd_attack << (j * 8);
				f_attack |= f_attack << j;
			}
			rank_diag_attack_table[i][occ] = rd_attack;
			file_attack_table[i][occ] = f_attack;
		}
	}
}

std::string show_bb(const BitBoard bb){
	std::string ret = "";
	for(int r = 7;r >= 0; r--){
		for(int f = 0; f < 8; f++){
			const Square sq = r * 8 + f;
			if(bb & bb_sq(sq))ret += "1";
			else ret += ".";
		}
		ret += "/";
	}
	return ret;
}
