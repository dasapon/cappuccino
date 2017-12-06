#include "bitboard.hpp"

BitBoardTable knight_attack_table, king_attack_table;
BitBoardTable rank_mask_table, file_mask_table, diag_mask_table, diag2_mask_table;
Array<BitBoardTable, PlayerDim> pawn_attack_table;
Array<BitBoardTable, NSquare> sandwiched_squares;

static Array<Array<BitBoard, 64>, 8> rank_diag_attack_table, file_attack_table;

void init_bitboard_tables(){
	
	auto left = [=](BitBoard bb){return (bb >> 1) & ~FILE_H;};
	auto right = [=](BitBoard bb){return (bb << 1) & ~FILE_A;};
	for(Square sq = 0;sq < NSquare;sq++){
		rank_mask_table[sq] = RANK_1 << (sq / 8);
		file_mask_table[sq] = FILE_A << (sq % 8);
		//init diag_mask and diag2_mask
		BitBoard diag1 = bb_sq(sq);
		while(true){
			BitBoard bb = ((diag1 << 9) & ~FILE_A) | ((diag1 >> 9) & ~FILE_H);
			if((bb | diag1) == diag1)break;
			diag1 |= bb;
		}
		diag_mask_table[sq] = diag1;
		BitBoard diag2 = bb_sq(sq);
		while(true){
			BitBoard bb = ((diag2 << 7) & ~FILE_H) | ((diag2 >> 7) & ~FILE_A);
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
		
		//init pawn_attack
		pawn_attack_table[White][sq] = lr << 8;
		pawn_attack_table[Black][sq] = lr >> 8;
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
	
	//init sandwiched squares
	for(Square sq1 = 0;sq1 < NSquare;sq1++){
		for(Square sq2 = 0; sq2 < NSquare; sq2++){
			BitBoard bb = bb_sq(sq1) | bb_sq(sq2);
			sandwiched_squares[sq1][sq2] = queen_attack(sq1, bb) | queen_attack(sq2, bb);
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

static inline BitBoard rank_diag_attack(Square sq, BitBoard occupied, BitBoard mask){
	occupied &= mask;
	return rank_diag_attack_table[file(sq)][(occupied * FILE_B) >> 58] & mask;
}

BitBoard bishop_attack(Square sq, BitBoard occupied){
	return rank_diag_attack(sq, occupied, diag_mask_table[sq])
		| rank_diag_attack(sq, occupied, diag2_mask_table[sq]);
}

BitBoard rook_attack(Square sq, BitBoard occupied){
	BitBoard rank_attack = rank_diag_attack(sq, occupied, rank_mask_table[sq]);
	//calculate file attack
	constexpr BitBoard magic = 0x0204081020408000ULL;
	/*
	 * H0000000		01000000	00BCDEFG
	 * G0000000		00100000	00000000
	 * F0000000		00010000	00000000
	 * E0000000		00001000	00000000
	 * D0000000		00000100	00000000
	 * C0000000		00000010	00000000
	 * B0000000		00000001	00000000
	 * A0000000		00000000	00000000
	 * */
	 BitBoard mask = file_mask_table[sq];
	 BitBoard file_attack = file_attack_table[rank(sq)][((mask >> file(sq)) * magic) >> 58] & mask;
	 return file_attack | rank_attack;
}

BitBoard queen_attack(Square sq, BitBoard occupied){
	return bishop_attack(sq, occupied) | rook_attack(sq, occupied);
}
