#pragma once

#include <cstdint>
#include <iostream>
#include <array>
#include <string>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <initializer_list>
#include <cstdint>
#include <thread>

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

inline std::string square_string(Square sq){
	assert(sq >= 0 && sq < 64);
	char c[3];
	c[0] = 'a' + file(sq);
	c[1] = '1' + rank(sq);
	c[2] = '\0';
	return std::string(c);
}

constexpr int MaxLegalMove = 256;


using FEN = Array<std::string, 6>;

class Position;
extern uint64_t perft(Position&, int);
extern bool unit_test_perft();
extern void uci_loop();

enum{
	ValueINF = 0xffff,
	MateValue = 32000,
	AlmostWin = 9999, 
	PawnValue = 100,
	KnightValue = 300,
	BishopValue = 300,
	RookValue = 500,
	QueenValue = 900,
};
extern const Array<int, PieceDim> material_value;
