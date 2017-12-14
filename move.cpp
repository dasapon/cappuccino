#include "position.hpp"
#include "bitboard.hpp"

bool Position::is_suicide_move(Move move)const{
	Square to = move.to();
	BitBoard to_bb = bb_sq(to);
	BitBoard ignored = to_bb;
	BitBoard next_all = all_bb ^ bb_sq(move.from());
	if(move.capture()!=Empty){
		if(to_bb == enpassant_bb){
			ignored ^= bb_sq(turn == White? to - 8 : to + 8);
			//next_all ^= to_bb;
			//next_all ^= bb_sq(turn == White? to - 8 : to + 8);
			next_all ^= ignored;
		}
	}
	else{
		next_all ^= to_bb;
	}
	Square ksq = king_sq[turn];
	if(move.piece() == King){
		ksq = to;
	}
	return is_attacked(opponent(turn), ksq, next_all, ignored);
}

bool Position::is_move_check(Move move)const{
	Square to = move.to();
	Square from = move.from();
	BitBoard to_bb = bb_sq(to);
	BitBoard from_bb = bb_sq(from);
	BitBoard next_all = all_bb ^ from_bb;
	const BitBoard bb_king = get_bb(opponent(turn), King);
	//castling
	if(move.is_castling()){
		//check by rook ?
		return (bb_king & piece_attack<Rook>((from + to) / 2, next_all)) != 0;
	}
	else{
		//direct check?
		switch(move.piece_moved()){
		case Pawn:
			if(pawn_attack_table[turn][to] & bb_king)return true;
			if(to_bb == enpassant_bb){
				next_all ^= bb_sq(turn == White? to - 8 : to + 8) ^ to_bb;
			}
			else if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case Knight:
			if(piece_attack<Knight>(to, next_all) & bb_king)return true;
			if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case Bishop:
			if(piece_attack<Bishop>(to, next_all) & bb_king)return true;
			if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case Rook:
			if(piece_attack<Rook>(to, next_all) & bb_king)return true;
			if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case Queen:
			if(piece_attack<Queen>(to, next_all) & bb_king)return true;
			if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case King:
			if(move.capture() == Empty)next_all ^= to_bb;
			break;
		case Empty:
		case PieceDim:
			assert(false);
		}
		//discovered check?
		return is_attacked(turn, king_sq[opponent(turn)], next_all, from_bb);
	}
}

bool Position::is_valid_move(Move move)const{
	Piece piece = move.piece();
	Piece capture = move.capture();
	Square from = move.from();
	Square to = move.to();
	BitBoard to_bb = bb_sq(to);
	BitBoard from_bb = bb_sq(from);
	if((get_bb(turn, piece) & from_bb) == 0)return false;
	if(move.piece() == Pawn){
		if(capture != Empty){
			if(to_bb == enpassant_bb)return capture == Pawn;
			else return (get_bb(opponent(turn), capture) & to_bb) != 0;
		}
		else{
			if(turn == White){
				BitBoard push = pawn_push<White>(from_bb, all_bb);
				push |= pawn_push<White>(push, all_bb);
				return (push & to_bb) != 0;
			}
			else{
				BitBoard push = pawn_push<Black>(from_bb, all_bb);
				push |= pawn_push<Black>(push, all_bb);
				return (push & to_bb) != 0;
			}
		}
	}
	else if(move.is_castling()){
		if(check())return false;
		CastlingFlag side = to > from ? Short : Long;
		return castling_flags[turn][side] 
			&& (all_bb & sandwiched_squares[from][rook_ini[turn][side]]) == 0
			&& !is_attacked(opponent(turn), (from + to) / 2);
		return true;
	}
	else {
		bool capture_is_ok = capture == Empty? ((all_bb & to_bb) == 0)
			: ((get_bb(opponent(turn), capture) & to_bb) != 0);
		return capture_is_ok
			&& (all_bb & sandwiched_squares[from][to]) == 0;
	}
}

int Position::see_sub(Player p, Square sq, BitBoard all, BitBoard ignored, int hanged)const{
	BitBoard bb = least_valuable_attacker(p, sq, all, ignored);
	if(bb){
		Square sq_ = bsf(bb);
		return std::max(0, hanged - see_sub(p, sq, all & ~bb, ignored | bb, material_value[board[sq_]]));
	}
	else return 0;
}

int Position::see(Move move)const{
	if(move.is_castling())return 0;
	int hanged = material_value[move.piece_moved()];
	int gain = hanged - material_value[move.piece()];
	BitBoard from_bb = bb_sq(move.from());
	Square to = move.to();
	Piece cap = move.capture();
	if(cap != Empty){
		return -see_sub(opponent(turn), to, all_bb & ~from_bb, from_bb, hanged) + material_value[cap] + gain;
	}
	else{
		BitBoard to_bb = bb_sq(to);
		return -see_sub(opponent(turn), to, (all_bb & ~from_bb) | to_bb, from_bb, hanged) + gain;
	}
}

Move::Move(const Position& pos, std::string str){
	Array<Move, MaxLegalMove> moves;
	int n = pos.generate_important_moves(moves, 0);
	n = pos.generate_unimportant_moves(moves, n);
	for(int i=0;i<n;i++){
		if(str == moves[i].to_lan()){
			move_ = moves[i].move_;
			return;
		}
	}
	std::cerr << "invalid move " << str << std::endl;
}

static std::string to_san(int idx, const Position& pos, Array<Move, MaxLegalMove>& moves, int n_moves){
	Move move = moves[idx];
	const Square to = move.to();
	const Square from = move.from();
	Piece piece = move.piece();
	std::string str("");
	if(move.is_castling()){
		if(to > from)str = "O-O";
		else str = "O-O-O";
	}
	else{
		if(piece == Pawn){
			if(move.capture() == Empty){
				str = square_string(to);
			}
			else{
				str += file_char(from);
				str += "x";
				str += square_string(to);
			}
			if(move.is_promotion()){
				str += "=";
				str += piece_char(White, move.piece_moved());
			}
		}
		else{
			str += piece_char(White, piece);
			bool to_deperture = true;
			bool file_deperture = true, rank_deperture = true;
			for(int i=0;i<n_moves;i++){
				if(i == idx)continue;
				Move m = moves[i];
				if(m.piece() == piece && m.to() == to){
					to_deperture = false;
					Square f = m.from();
					file_deperture &= (file(from) != file(f));
					rank_deperture &= (rank(from) != rank(f));
				}
			}
			if(!to_deperture){
				if(file_deperture)str += file_char(from);
				else if(rank_deperture)str += rank_char(from);
				else str += square_string(from);
			}
			if(move.capture() != Empty)str += "x";
			str += square_string(to);
		}
	}
	if(pos.is_move_check(move)){
		Position next(pos);
		next.make_move(move);
		Array<Move, MaxLegalMove> move_next;
		int n_next_moves = next.generate_important_moves(move_next, 0);
		bool mate = true;
		for(int i=0;i<n_next_moves;i++){
			if(!next.is_suicide_move(move_next[i])){
				mate = false;break;
			}
		}
		if(mate)str += "#";
		else str += "+";
	}
	return str;
}

Move san2move(const Position& pos, std::string str){
	//move generation
	Array<Move, MaxLegalMove> moves;
	int n_moves = pos.generate_important_moves(moves, 0);
	n_moves = pos.generate_unimportant_moves(moves, n_moves);
	for(int i=0;i<n_moves;i++)if(pos.is_suicide_move(moves[i]))moves[i--] = moves[--n_moves];
	for(int i=0;i<n_moves;i++){
		std::string san = to_san(i, pos, moves, n_moves);
		if(str == san){
			return moves[i];
		}
	}
	std::cerr << "invalid move " << str << std::endl;
	return NullMove;
}
