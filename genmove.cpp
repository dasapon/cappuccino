#include "position.hpp"
#include "bitboard.hpp"

template<Piece piece>
int Position::generate_piece_moves(Array<Move, MaxLegalMove>& moves, int idx, BitBoard target)const{
	BitBoard from_bb = occupied[turn] & pieces[piece];
	while(from_bb){
		Square from = pop_one(from_bb);
		BitBoard to_bb = piece_attack<piece>(from, all_bb) & target;
		while(to_bb){
			Square to = pop_one(to_bb);
			moves[idx++] = Move(piece, board[to], from, to);
		}
	}
	return idx;
}
template<Player us>
int Position::generate_pawn_moves(Array<Move, MaxLegalMove>& moves, int idx, BitBoard target)const{
	assert(us == turn);
	const BitBoard enemy_pieces = occupied[opponent(us)];
	constexpr BitBoard promotion_squares = us ==White? RANK_8 : RANK_1;
	const BitBoard pawns = occupied[us] & pieces[Pawn];
	const BitBoard attack_west = pawn_attack_west<us>(pawns) & target;
	const BitBoard attack_east = pawn_attack_east<us>(pawns) & target;
	//Generate captures
	BitBoard to_bb = attack_west & enemy_pieces & ~promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = turn==White? to - 7 : to + 9;
		moves[idx++] = Move(Pawn, board[to], from, to);
	}
	to_bb = attack_west & enemy_pieces & promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us==White? to - 7 : to + 9;
		moves[idx++] = Move(Pawn, Knight, board[to], from, to);
		moves[idx++] = Move(Pawn, Bishop, board[to], from, to);
		moves[idx++] = Move(Pawn, Rook, board[to], from, to);
		moves[idx++] = Move(Pawn, Queen, board[to], from, to);
	}
	to_bb = attack_east & enemy_pieces & ~promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us==White? to - 9 : to + 7;
		moves[idx++] = Move(Pawn, board[to], from, to);
	}
	to_bb = attack_east & enemy_pieces & promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us==White? to - 9 : to + 7;
		moves[idx++] = Move(Pawn, Knight, board[to], from, to);
		moves[idx++] = Move(Pawn, Bishop, board[to], from, to);
		moves[idx++] = Move(Pawn, Rook, board[to], from, to);
		moves[idx++] = Move(Pawn, Queen, board[to], from, to);
	}
	//Generate enpassant
	if(target & enpassant_bb){
		Square to = bsf(enpassant_bb);
		if(attack_west & enpassant_bb){
			Square from = turn==White? to - 7 : to + 9;
			moves[idx++] = Move(Pawn, Pawn, from, to);
		}
		if(attack_east & enpassant_bb){
			Square from = us==White? to - 9 : to + 7;
			moves[idx++] = Move(Pawn, Pawn, from, to);
		}
	}
	//Generate pawn push
	BitBoard push = pawn_push<us>(pawns, all_bb);
	to_bb =  push & target & promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us == White? to - 8 : to + 8;
		moves[idx++] = Move(Pawn, Knight, Empty, from, to);
		moves[idx++] = Move(Pawn, Bishop, Empty, from, to);
		moves[idx++] = Move(Pawn, Rook, Empty, from, to);
		moves[idx++] = Move(Pawn, Queen, Empty, from, to);
	}
	to_bb = push & target & ~promotion_squares;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us == White? to - 8 : to + 8;
		moves[idx++] = Move(Pawn, Empty, from, to);
	}
	//generate pawn push2
	to_bb = pawn_push<us>(push, all_bb) & (us==White? RANK_4 : RANK_5) & target;
	while(to_bb){
		Square to = pop_one(to_bb);
		Square from = us == White? to - 16 : to + 16;
		moves[idx++] = Move(Pawn, Empty, from, to);
	}
	return idx;
}
int Position::generate_important_moves(Array<Move, MaxLegalMove>& moves,int n_moves)const{
	if(check()){
		//Generate all moves when check
		Square ksq = king_sq[turn];
		//evasion king
		n_moves = generate_piece_moves<King>(moves, n_moves, ~occupied[turn]);
		BitBoard checking_pieces = attackers(opponent(turn), ksq);
		if(more_than_one(checking_pieces))return n_moves;
		//capture or prevent sliding_attack
		BitBoard target = checking_pieces | sandwiched_squares[ksq][bsf(checking_pieces)];
		n_moves = generate_piece_moves<Knight>(moves, n_moves, target);
		n_moves = generate_piece_moves<Bishop>(moves, n_moves, target);
		n_moves = generate_piece_moves<Rook>(moves, n_moves, target);
		n_moves = generate_piece_moves<Queen>(moves, n_moves, target);
		//pawns' move
		if(board[bsf(target)] == Pawn){
			target |= enpassant_bb;
		}
		if(turn == White)n_moves = generate_pawn_moves<White>(moves, n_moves, target);
		else n_moves = generate_pawn_moves<Black>(moves, n_moves, target);
		return n_moves;
	}
	//Generate captures
	BitBoard target = occupied[opponent(turn)];
	
	n_moves = generate_piece_moves<Knight>(moves, n_moves, target);
	n_moves = generate_piece_moves<Bishop>(moves, n_moves, target);
	n_moves = generate_piece_moves<Rook>(moves, n_moves, target);
	n_moves = generate_piece_moves<Queen>(moves, n_moves, target);
	n_moves = generate_piece_moves<King>(moves, n_moves, target);
	//Generate pawns' move
	if(turn == White)n_moves = generate_pawn_moves<White>(moves, n_moves, target | RANK_8 | enpassant_bb);
	else n_moves = generate_pawn_moves<Black>(moves, n_moves, target | RANK_1 | enpassant_bb);
	//Generate castling
	Square ksq = king_sq[turn];
	if(castling_flags[turn][CastlingFlag::Short]){
		BitBoard sandwiched = sandwiched_squares[ksq][rook_ini[turn][CastlingFlag::Short]];
		if((sandwiched & all_bb) == 0
			&& !is_attacked(opponent(turn), ksq + 1)){
			moves[n_moves++] = Move(King, Empty, ksq, ksq + 2);
		}
	}
	if(castling_flags[turn][CastlingFlag::Long]){
		BitBoard sandwiched = sandwiched_squares[ksq][rook_ini[turn][CastlingFlag::Long]];
		if((sandwiched & all_bb) == 0
			&& !is_attacked(opponent(turn), ksq - 1)){
			moves[n_moves++] = Move(King, Empty, ksq, ksq - 2);
		}
	}
	return n_moves;
}
int Position::generate_unimportant_moves(Array<Move, MaxLegalMove>& moves, int n_moves)const{
	if(check())return n_moves;
	BitBoard target = ~all_bb;
	n_moves = generate_piece_moves<Knight>(moves, n_moves, target);
	n_moves = generate_piece_moves<Bishop>(moves, n_moves, target);
	n_moves = generate_piece_moves<Rook>(moves, n_moves, target);
	n_moves = generate_piece_moves<Queen>(moves, n_moves, target);
	n_moves = generate_piece_moves<King>(moves, n_moves, target);
	//Generate pawns' move
	if(turn == White)n_moves = generate_pawn_moves<White>(moves, n_moves, target & ~RANK_8 & ~enpassant_bb);
	else n_moves = generate_pawn_moves<Black>(moves, n_moves, target & ~RANK_1 & ~enpassant_bb);
	return n_moves;
}
