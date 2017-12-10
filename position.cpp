#include "position.hpp"
#include "bitboard.hpp"


const Square Position::rook_ini[PlayerDim][CastlingFlagDim] = {{7, 0}, {63, 56}};
Array<Array<Array<uint64_t, NSquare>, PieceDim>, PlayerDim> Position::hash_seed;
const FEN startpos({"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR","w","KQkq","-","0","1"});
void Position::xor_piece(Player us, Piece piece, Square sq){
	assert(board[sq] == piece || board[sq] == Empty);
	board[sq] ^= piece;
	BitBoard bb = bb_sq(sq);
	all_bb ^= bb;
	occupied[us] ^= bb;
	pieces[piece] ^= bb;
	hash_key ^= hash_seed[us][piece][sq];
}

bool Position::is_attacked(Player p, Square sq)const{
	return is_attacked(p, sq, all_bb, 0);
}
bool Position::is_attacked(Player p, Square sq, BitBoard customized_all, BitBoard ignored)const{
	if(piece_attack<Knight>(sq, customized_all) & occupied[p] & pieces[Knight] & ~ignored)return true;
	if(piece_attack<King>(sq, customized_all) & occupied[p] & pieces[King]& ~ignored)return true;
	if(piece_attack<Rook>(sq, customized_all) & occupied[p] & (pieces[Rook] | pieces[Queen]) & ~ignored)return true;
	if(piece_attack<Bishop>(sq, customized_all) & occupied[p] & (pieces[Bishop] | pieces[Queen]) & ~ignored)return true;
	return (pawn_attack_table[opponent(p)][sq] & occupied[p] & pieces[Pawn] & ~ignored) != 0;
}
BitBoard Position::attackers(Player us, Square sq)const{
	BitBoard ret = piece_attack<Knight>(sq, all_bb) & occupied[us] & pieces[Knight];
	ret |= piece_attack<King>(sq, all_bb) & occupied[us] & pieces[King];
	ret |= piece_attack<Rook>(sq, all_bb) & occupied[us] & (pieces[Rook] | pieces[Queen]);
	ret |= piece_attack<Bishop>(sq, all_bb) & occupied[us] & (pieces[Bishop] | pieces[Queen]);
	ret |= pawn_attack_table[opponent(us)][sq] & occupied[us] & pieces[Pawn];
	return ret;
}

void Position::make_move(Move move){
	Square from = move.from(), to = move.to();
	Piece captured = move.capture();
	//remove captured piece
	if(captured != Empty){
		if(bb_sq(to) == enpassant_bb){//enpassant
			assert(captured == Pawn);
			if(turn == White){
				xor_piece(Black, captured, to - 8);
			}
			else{
				xor_piece(White, captured, to + 8);
			}
		}
		else{//normal capture
			xor_piece(opponent(turn), captured, to);
		}
	}
	//remove piece
	xor_piece(turn, move.piece(), from);
	//put piece_moved
	xor_piece(turn, move.piece_moved(), to);
	if(move.is_castling()){
		//Move Rook
		Square rook_to = (from + to) / 2;
		Square rook_from = rook_ini[turn][to > from? CastlingFlag::Short : CastlingFlag::Long];
		xor_piece(turn, Rook, rook_from);
		xor_piece(turn, Rook, rook_to);
	}
	//set flags
	enpassant_bb = 0;
	if(move.piece() == King){
		castling_flags[turn][CastlingFlag::Short] = false;
		castling_flags[turn][CastlingFlag::Long] = false;
		king_sq[turn] = to;
	}
	else if(move.piece() == Rook){
		for(int castle_flag = 0;castle_flag!=CastlingFlagDim;castle_flag++){
			Square sq = rook_ini[turn][castle_flag];
			if(from == sq){
				castling_flags[turn][castle_flag] = false;
			}
		}
	}
	else if(move.piece() == Pawn){
		if(std::abs(from - to) == 16){
			enpassant_bb = bb_sq((from + to) / 2);
		}
	}
	if(move.capture() == Rook){
		for(int castle_flag = 0;castle_flag!=CastlingFlagDim;castle_flag++){
			Square sq = rook_ini[opponent(turn)][castle_flag];
			if(to == sq){
				castling_flags[opponent(turn)][castle_flag] = false;
			}
		}
	}
	turn = opponent(turn);
	hash_key ^= 1ULL;
	if(captured != Empty || move.piece() == Pawn)half_move_counter = 0;
	else half_move_counter++;
}

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

void Position::load_fen(const FEN& fen){
	//zero clear
	occupied[White] = occupied[Black] = all_bb = 0;
	for(Piece p = Pawn;p != PieceDim;p++){
		pieces[p] = 0;
	}
	for(Square i=0;i<NSquare;i++)board[i] = Empty;
	hash_key = 0;
	castling_flags[White][CastlingFlag::Short] = false;
	castling_flags[White][CastlingFlag::Long] = false;
	castling_flags[Black][CastlingFlag::Short] = false;
	castling_flags[Black][CastlingFlag::Long] = false;
	enpassant_bb = 0;
	//init board
	int f = 0;int r = 7;
	for(char c:fen[0]){
		assert(f < 8);
		assert(r >= 0);
		if(c == '/'){
			r--;
			f=0;
		}
		else if('0' < c && c < '9'){
			f += c - '0';
		}
		else{
			for(Piece p = Pawn;p != PieceDim;p++){
				if(c == piece_char(White, p))xor_piece(White, p, r * 8 + f);
				else if(c == piece_char(Black, p))xor_piece(Black, p, r * 8 + f);
			}
			f++;
		}
	}
	//init king_sq
	king_sq[White] = bsf(get_bb(White, King));
	king_sq[Black] = bsf(get_bb(Black, King));
	//init turn
	if(fen[1] == "w"){
		turn = White;
	}
	else{
		turn = Black;
	}
	//init castling flags
	for(char c:fen[2]){
		if(c == 'K')castling_flags[White][CastlingFlag::Short] = true;
		if(c == 'Q')castling_flags[White][CastlingFlag::Long] = true;
		if(c == 'k')castling_flags[Black][CastlingFlag::Short] = true;
		if(c == 'q')castling_flags[Black][CastlingFlag::Long] = true;
	}
	//init enpassant flags
	for(Square sq = 0;sq < NSquare; sq++){
		if(square_string(sq) == fen[3]){
			enpassant_bb = bb_sq(sq);
		}
	}
	//init halfmove count
	half_move_counter = std::stoi(fen[4]);
	//fullmove count(skip)
}

Move Position::str2move(std::string move_str)const{
	Array<Move, MaxLegalMove> moves;
	int n = generate_important_moves(moves, 0);
	n = generate_unimportant_moves(moves, n);
	for(int i=0;i<n;i++){
		if(move_str == moves[i].to_fen())return moves[i];
	}
	return NullMove;
}

void Position::init_hash_seed(){
	std::mt19937 mt(0);
	for(Piece p = Empty; p != PieceDim;p++){
		for(Square sq = 0;sq < NSquare; sq++){
			hash_seed[White][p][sq] = mt() & ~1ULL;
			hash_seed[Black][p][sq] = mt() & ~1ULL;
		}
	}
}

bool Position::immediately_draw()const{
	if(half_move_counter >= 100)return true;
	int n = popcnt(all_bb);
	return n == 2 || (n == 3 && (pieces[Knight] | pieces[Bishop]) != 0);
}
