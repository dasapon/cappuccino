#pragma once

#include <cstdint>
#include <climits>
#include <cfloat>
#include <iostream>
#include <array>
#include <string>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <initializer_list>
#include <thread>
#include <random>

#include "sheena/sheena.hpp"

using BitBoard = uint64_t;
using Square = int32_t;
class Move;

enum Player{
	White,Black,PlayerDim,
};
inline Player opponent(Player p){return static_cast<Player>(1 - p);}
inline void operator++(Player& p, int){
	p = static_cast<Player>(p + 1);
}
enum Piece{
	Empty, Pawn, Knight, Bishop, Rook, Queen, King, PieceDim,
};

inline void operator^=(Piece& p1, Piece p2){
	p1 = static_cast<Piece>(p1 ^ p2);
}
inline void operator++(Piece& p, int){
	p = static_cast<Piece>(p + 1);
}
inline char piece_char(Player us, Piece p){
	const std::string s = "XPNBRQK";
	if(us==White)return s.at(p);
	else return s.at(p) + 32;
}

constexpr int NSquare = 64;

inline int file(Square sq){return sq % 8;}
inline int rank(Square sq){return sq / 8;}
inline Square wb_reverse(Square sq){
	return file(sq) + (7 - rank(sq)) * 8;
}

inline char file_char(Square sq){
	return 'a' + file(sq);
}
inline char rank_char(Square sq){
	return '1' + rank(sq);
}

inline std::string square_string(Square sq){
	assert(sq >= 0 && sq < 64);
	char c[3];
	c[0] = file_char(sq);
	c[1] = rank_char(sq);
	c[2] = '\0';
	return std::string(c);
}

constexpr int MaxLegalMove = 256;


using FEN = sheena::Array<std::string, 6>;

class Position;
extern uint64_t perft(Position&, int);
extern bool unit_test_perft();
extern bool unit_test_see();
extern void search_test();
extern void uci_loop();

enum{
	ValueINF = 0x7fff,
	MateValue = 32000,
	AlmostWin = 9999, 
	PawnValue = 100,
	KnightValue = 350,
	BishopValue = 350,
	RookValue = 525,
	QueenValue = 1000,
};
constexpr int depth_scale = 8;
extern const sheena::Array<int, PieceDim> material_value;

enum : int32_t{
	pawn_index = -8,
	knight_index = pawn_index + 64 - 8,
	bishop_index = knight_index + 64,
	rook_index = bishop_index + 64,
	queen_index = rook_index + 64,
	king_index = queen_index + 64,
	friend_piece_list_dim = king_index + 64,
	enemy_pawn_index = friend_piece_list_dim - 8,
	enemy_knight_index = enemy_pawn_index + 64 - 8,
	enemy_bishop_index = enemy_knight_index + 64,
	enemy_rook_index = enemy_bishop_index + 64,
	enemy_queen_index = enemy_rook_index + 64,
	enemy_king_index = enemy_queen_index + 64,
	piece_list_dim = enemy_king_index + 64,
};

class PieceListIndex{
	static sheena::Array<PieceListIndex, NSquare> sq_index;
	static sheena::Array2d<PieceListIndex, PieceDim, PlayerDim> piece_index;
	//
	union{
		int32_t w32[2];
		int64_t w64;
	};
public:
	static void init_table();
	PieceListIndex():w64(0){};
	PieceListIndex(const PieceListIndex& pli):w64(pli.w64){};
	void operator=(const PieceListIndex& rhs){
		w64 = rhs.w64;
	}
	PieceListIndex(Player owner, Piece piece, Square sq){
		w64 = piece_index[piece][owner].w64 + sq_index[sq].w64;
	}
	template<Player side>
	int index()const{
		return w32[side];
	}
};

struct Record : public std::vector<Move>{
	int result;
};

extern std::vector<std::string> split(std::string str);
