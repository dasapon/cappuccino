#pragma once

#include "cappuccino.hpp"

//Move
//xxxx xxxx xxxx xxxx xxxx xxxx xx11 1111	to
//xxxx xxxx xxxx xxxx xxxx 1111 11xx xxxx	from
//xxxx xxxx xxxx xxxx 1111 xxxx xxxx xxxx   piece
//xxxx xxxx xxxx 1111 xxxx xxxx xxxx xxxx   piece_moved
//xxxx xxxx 1111 xxxx xxxx xxxx xxxx xxxx	capture
class Move{
	int move_;
	Move(int i):move_(i){}
public:
	static Move NullMove(){return Move(0);}
	Move(){}
	Move(Piece piece, Piece capture, Square from, Square to){
		move_ = (piece << 12) | (piece << 16) | (capture << 20) | (from << 6) | to;
	}
	Move(Piece piece, Piece promotion, Piece capture, Square from, Square to){
		move_ = (piece << 12) | (promotion << 16) | (capture << 20) | (from << 6) | to;
	}
	Square to()const{
		return static_cast<Square>(move_ & 0x3f);
	}
	Square from()const{
		return static_cast<Square>((move_ >> 6) & 0x3f);
	}
	Piece piece()const{
		return static_cast<Piece>((move_ >> 12) & 0xf);
	}
	Piece piece_moved()const{
		return static_cast<Piece>((move_ >> 16) & 0xf);
	}
	Piece capture()const{
		return static_cast<Piece>((move_ >> 20) & 0xf);
	}
	bool is_promotion()const{
		return piece() != piece_moved();
	}
	bool is_null()const{
		return move_ == 0;
	}
	bool is_castling()const{
		return piece() == King && std::abs(from() - to()) == 2;
	}
	std::string to_fen()const{
		if(is_promotion()){
			return square_string(from()) + square_string(to()) + piece_char(Black, piece_moved());
		}
		else{
			return square_string(from()) + square_string(to());
		}
	}
};