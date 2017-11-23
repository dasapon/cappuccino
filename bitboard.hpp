#pragma once

#include "cappuccino.hpp"

template<size_t Size>
using BitBoardTable = Array<BitBoard, Size>;

extern BitBoardTable<64> knight_attack_table, king_attack_table;
extern BitBoardTable<64> rank_mask_table, file_mask_table, diag_mask_table, diag2_mask_table;

extern void init_tables();

extern BitBoard bishop_attack(Square sq, BitBoard empty);
extern BitBoard rook_attack(Square sq, BitBoard empty);
extern BitBoard queen_attack(Square sq, BitBoard empty);

extern std::string show_bb(const BitBoard bb);

inline BitBoard bb_sq(Square sq){
	return static_cast<BitBoard>(1ULL << sq);
}
