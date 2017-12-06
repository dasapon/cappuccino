#pragma once

#include "move.hpp"


extern const FEN startpos;
class Position{
	enum CastlingFlag{
		Short, Long, CastlingFlagDim,
	};
	static const Square rook_ini[PlayerDim][CastlingFlagDim];
	static Array<Array<Array<uint64_t, NSquare>, PieceDim>, PlayerDim> hash_seed;
	
	Array<BitBoard, PlayerDim> occupied;
	Array<BitBoard, PieceDim> pieces;
	BitBoard all_bb;
	Array<Piece, NSquare> board;
	uint64_t hash_key;
	Array<Square, PlayerDim> king_sq;
	int half_move_counter;
	Player turn;
	Array<Array<bool, PlayerDim>, CastlingFlagDim> castling_flags;
	BitBoard enpassant_bb;
	BitBoard get_bb(Player turn, Piece piece){return occupied[turn] & pieces[piece];}
	void xor_piece(Player turn, Piece piece, Square sq);
	template<Piece piece>
	int generate_piece_moves(Array<Move, MaxLegalMove>&, int idx, const BitBoard target)const;
	template<Player turn>
	int generate_pawn_moves(Array<Move, MaxLegalMove>&, int idx, BitBoard target)const;
	bool is_attacked(Player p, Square sq)const;
	bool is_attacked(Player p, Square sq, BitBoard customized_all, BitBoard ignored)const;
	BitBoard attackers(Player turn, Square sq)const;
public:
	static void init_hash_seed();
	BitBoard empty_bb()const{return ~(occupied[White] | occupied[Black]);}
	int generate_important_moves(Array<Move, MaxLegalMove>&, int)const;
	int generate_unimportant_moves(Array<Move, MaxLegalMove>&, int)const;
	void make_move(Move move);
	void operator=(const Position& pos){memcpy(this,&pos,sizeof(Position));}
	void load_fen(const FEN& fen);
	Move str2move(std::string move_str)const;
	Position(const Position& pos){(*this) = pos;}
	Position(const FEN& fen){load_fen(fen);}
	Position(){load_fen(startpos);}
	Player turn_player()const{return turn;}
	bool check()const{return is_attacked(opponent(turn), king_sq[turn]);}
	bool is_suicide_move(Move move) const;
	FEN to_fen()const;
};
