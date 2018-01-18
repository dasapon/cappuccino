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

template<typename Ty, size_t Size>
class Array{
	Ty array_[Size];
	void assert_(int idx)const{assert(idx >= 0);assert(idx < Size);}
public:
	Array(){}
	explicit Array(const std::initializer_list<Ty> init){
		auto itr = init.begin();
		for(int i=0;i<Size && i < init.size();i++){
			array_[i] = *itr;
			itr++;
		}
	}
	static size_t size(){return Size;}
	Ty operator[](int idx)const{
		assert_(idx);
		return array_[idx];
	}
	Ty& operator[](int idx){
		assert_(idx);
		return array_[idx];
	}
	void operator=(const Array<Ty, Size> & a){
		for(int i=0;i<Size;i++)(*this)[i] = a[i];
	}
};

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
	PassedPawn = 0,
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


using FEN = Array<std::string, 6>;

class Position;
extern uint64_t perft(Position&, int);
extern bool unit_test_perft();
extern bool unit_test_see();
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
extern const Array<int, PieceDim> material_value;

enum {
	passed_pawn_index = -8,
	pawn_index = passed_pawn_index + 64 - 16,
	knight_index = pawn_index + 64 - 8,
	bishop_index = knight_index + 64,
	rook_index = bishop_index + 64,
	queen_index = rook_index + 64,
	king_index = queen_index + 64,
	friend_piece_index_dim = king_index + 64,
	enemy_passed_pawn_index = friend_piece_index_dim - 8,
	enemy_pawn_index = enemy_passed_pawn_index + 64 - 16,
	enemy_knight_index = enemy_pawn_index + 64 - 8,
	enemy_bishop_index = enemy_knight_index + 64,
	enemy_rook_index = enemy_bishop_index + 64,
	enemy_queen_index = enemy_rook_index + 64,
	enemy_king_index = enemy_queen_index + 64,
	piece_none_index = enemy_king_index + 64,
	piece_index_dim = piece_none_index + 32,
};

template <bool is_enemy>
extern int piece_index(Piece p, Square sq, Player side);

using Record = std::vector<Move>;
extern std::vector<std::string> split(std::string str);
extern std::vector<Record> read_pgn(std::string file_name, int elo);
