#pragma once

#include <cstdint>
#include <iostream>
#include <array>
#include <string>
#include <cassert>

using BitBoard = uint64_t;
using Square = uint32_t;

enum Player{
	White,Black,PlayerDim,
};
enum Piece{
	Empty, Pawn, Knight, Bishop, Rook, Queen, King, PieceDim,
};
constexpr int NSquare = 64;

template<typename Ty, size_t Size>
class Array{
	Ty array_[Size];
	void assert_(int idx)const{assert(idx >= 0);assert(idx < Size);}
public:
	Ty operator[](int idx)const{
		assert_(idx);
		return array_[idx];
	}
	Ty& operator[](int idx){
		assert_(idx);
		return array_[idx];
	}
};
