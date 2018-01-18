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
	if(piece_attack<Knight>(sq, customized_all) & get_bb(p, Knight) & ~ignored)return true;
	if(piece_attack<King>(sq, customized_all) & get_bb(p, King) & ~ignored)return true;
	if(piece_attack<Rook>(sq, customized_all) & occupied[p] & (pieces[Rook] | pieces[Queen]) & ~ignored)return true;
	if(piece_attack<Bishop>(sq, customized_all) & occupied[p] & (pieces[Bishop] | pieces[Queen]) & ~ignored)return true;
	return (pawn_attack_table[opponent(p)][sq] & get_bb(p, Pawn) & ~ignored) != 0;
}
BitBoard Position::least_valuable_attacker(Player p, Square sq, BitBoard customized_all, BitBoard ignored)const{
	BitBoard bb = pawn_attack_table[opponent(p)][sq] & get_bb(p, Pawn) & ~ ignored;
	if(bb) return bb & -bb;
	bb = piece_attack<Knight>(sq, customized_all) & get_bb(p, Knight) & ~ ignored;
	if(bb) return bb & -bb;
	bb = piece_attack<Bishop>(sq, customized_all) & get_bb(p, Bishop) & ~ ignored;
	if(bb) return bb & -bb;
	bb = piece_attack<Rook>(sq, customized_all) & get_bb(p, Rook) & ~ ignored;
	if(bb) return bb & -bb;
	bb = piece_attack<Queen>(sq, customized_all) & get_bb(p, Queen) & ~ ignored;
	if(bb) return bb & -bb;
	bb = piece_attack<King>(sq, customized_all) & get_bb(p, King) & ~ ignored;
	return bb & -bb;
}
BitBoard Position::attackers(Player us, Square sq)const{
	BitBoard ret = piece_attack<Knight>(sq, all_bb) & get_bb(us, Knight);
	ret |= piece_attack<King>(sq, all_bb) & get_bb(us, King);
	ret |= piece_attack<Rook>(sq, all_bb) & occupied[us] & (pieces[Rook] | pieces[Queen]);
	ret |= piece_attack<Bishop>(sq, all_bb) & occupied[us] & (pieces[Bishop] | pieces[Queen]);
	ret |= pawn_attack_table[opponent(us)][sq] & get_bb(us, Pawn);
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
		assert(f < 8 || (f == 8 && c == '/'));
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

void Position::init_hash_seed(){
	std::mt19937_64 mt(0);
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

static const Array<int, PieceDim> piece_index_table({
	passed_pawn_index, pawn_index, knight_index, bishop_index, 
	rook_index, queen_index, king_index,
});

static const Array<int, PieceDim> enemy_piece_index_table({
	enemy_passed_pawn_index, enemy_pawn_index, enemy_knight_index, enemy_bishop_index, 
	enemy_rook_index, enemy_queen_index, enemy_king_index,
});

template <bool is_enemy>
int piece_index(Piece p, Square sq, Player side){
	if(is_enemy)return enemy_piece_index_table[p] + (side == White? sq : wb_reverse(sq));
	else return piece_index_table[p] + (side == White? sq : wb_reverse(sq));
}
template int piece_index<true>(Piece p, Square sq, Player side);
template int piece_index<false>(Piece p, Square sq, Player side);
int Position::piece_list(Array<int, 32>& list)const{
	int ret = 0;
	for(Piece p = Knight;p < PieceDim;p++){
		BitBoard bb = get_bb(turn, p);
		while(bb){
			Square sq = pop_one(bb);
			list[ret++] = piece_index<false>(p, sq, turn);
		}
		bb = get_bb(opponent(turn), p);
		while(bb){
			Square sq = pop_one(bb);
			list[ret++] = piece_index<true>(p, sq, turn);
		}
	}
	//pawns and passed pawns
	BitBoard bb = get_bb(turn, Pawn);
	BitBoard enemy_pawns = get_bb(opponent(turn), Pawn);
	while(bb){
		Square sq = pop_one(bb);
		if(enemy_pawns & forward3_table[turn][sq])
			list[ret++] = piece_index<false>(Pawn, sq, turn);
		else
			list[ret++] = piece_index<false>(PassedPawn, sq, turn);
	}
	bb = enemy_pawns;
	enemy_pawns = get_bb(turn, Pawn);
	while(bb){
		Square sq = pop_one(bb);
		if(enemy_pawns & forward3_table[opponent(turn)][sq])
			list[ret++] = piece_index<true>(Pawn, sq, turn);
		else
			list[ret++] = piece_index<true>(PassedPawn, sq, turn);
	}
	if(ret < 32){
		list[ret] = piece_none_index + 31 - ret;
		ret++;
	}
	return ret;
}
