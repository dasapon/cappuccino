#pragma once

#include "cappuccino.hpp"

using BitBoardTable = Array<BitBoard, 64>;

constexpr BitBoard FILE_A = 0x0101010101010101ULL;
constexpr BitBoard FILE_B = FILE_A << 1;
constexpr BitBoard FILE_C = FILE_A << 2;
constexpr BitBoard FILE_D = FILE_A << 3;
constexpr BitBoard FILE_E = FILE_A << 4;
constexpr BitBoard FILE_F = FILE_A << 5;
constexpr BitBoard FILE_G = FILE_A << 6;
constexpr BitBoard FILE_H = FILE_A << 7;

constexpr BitBoard RANK_1 = 0xffULL;
constexpr BitBoard RANK_2 = RANK_1 << 8;
constexpr BitBoard RANK_3 = RANK_1 << 16;
constexpr BitBoard RANK_4 = RANK_1 << 24;
constexpr BitBoard RANK_5 = RANK_1 << 32;
constexpr BitBoard RANK_6 = RANK_1 << 40;
constexpr BitBoard RANK_7 = RANK_1 << 48;
constexpr BitBoard RANK_8 = RANK_1 << 56;

extern BitBoardTable knight_attack_table, king_attack_table;
extern BitBoardTable rank_mask_table, file_mask_table, diag_mask_table, diag2_mask_table;
extern Array<BitBoardTable, PlayerDim> pawn_attack_table;
extern Array<BitBoardTable, NSquare> sandwiched_squares;

inline Square bsf(BitBoard bb){
#ifdef _WIN64
	unsigned long ret = 0;
	_BitScanForward64(&ret, bb);
#elif defined(__x86_64__)
	Square ret = __builtin_ctzll(bb);
#endif
	return ret;
}
inline Square pop_one(BitBoard& bb){
	Square ret = bsf(bb);
	bb &= bb - 1;
	return ret;
}
inline bool more_than_one(BitBoard bb){
	return (bb & (bb - 1)) != 0;
}

extern void init_bitboard_tables();

template<Player turn>
BitBoard pawn_attack_west(BitBoard bb){
	if(turn == White)return (bb << 7) & ~FILE_H;
	else return (bb >> 9) & ~FILE_H;
}
template<Player turn>
BitBoard pawn_attack_east(BitBoard bb){
	if(turn == White)return (bb << 9) & ~FILE_A;
	else return (bb >> 7) & ~FILE_A;
}
template<Player turn>
BitBoard pawn_push(BitBoard pawns, BitBoard occupied){
	if(turn == White)return (pawns << 8) & ~occupied;
	else return (pawns >> 8) & ~occupied;
}
extern BitBoard bishop_attack(Square sq, BitBoard occupied);
extern BitBoard rook_attack(Square sq, BitBoard occupied);
extern BitBoard queen_attack(Square sq, BitBoard occupied);

extern std::string show_bb(const BitBoard bb);

template<Piece piece>
extern BitBoard piece_attack(Square sq, BitBoard occupied){
	switch(piece){
	case Pawn:
		assert(false);
	case Knight:
		return knight_attack_table[sq];
	case Bishop:
		return bishop_attack(sq, occupied);
	case Rook:
		return rook_attack(sq, occupied);
	case Queen:
		return queen_attack(sq, occupied);
	case King:
		return king_attack_table[sq];
	default:
		assert(false);
		return 0;
	}
}

inline BitBoard bb_sq(Square sq){
	return static_cast<BitBoard>(1ULL << sq);
}
