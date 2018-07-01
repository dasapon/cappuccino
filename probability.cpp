#include "probability.hpp"
#include "time.hpp"
#include "learn.hpp"

enum{
	SeeMinus,
	Castling,
	Check,
	Capture,
	Promotion = Capture + King - Knight,
	Escape = Promotion + King - 2 * Pawn,
	OpenEffect = Escape + 2 * PieceDim,
	To,
	From = To + friend_piece_index_dim * piece_index_dim,
	PreviousTo = From + friend_piece_index_dim * piece_index_dim,
	PreviousFrom = PreviousTo + friend_piece_index_dim * piece_index_dim,
	ReCapture = PreviousFrom + friend_piece_index_dim * piece_index_dim,
	Dim,
};

using Weights = sheena::Array<sheena::Float4, Dim>;

static Weights weights;

void load_proabiblity(){
	FILE * fp = fopen("probability.bin", "rb");
	bool fail = true;
	if(fp != NULL){
		fail = fread(&weights, sizeof(Weights), 1, fp) != 1;
		fclose(fp);
	}
	if(fail){
		std::cout << "probabiblity loading is failed." << std::endl;
	}
}

template<bool update>
static void proce(int idx, Weights& w, sheena::Float4& d){
	if(update)w[idx] += d;
	else d += w[idx];
}

template<bool update>
static float proce(const State& state, Move move, Weights& w, float d){
	sheena::Float4 flt4 = 0;
	const Position& pos = state.pos();
	int n_pieces;
	const sheena::Array<int, 32>& piece_list = state.get_piece_list(&n_pieces);
	int see = pos.see(move);
	bool check = pos.is_move_check(move);
	Piece piece = move.piece();
	Piece capture = move.capture();
	const Player turn = pos.turn_player();
	Square to = move.to();
	Square from = move.from();
	sheena::Float4 coefficients(1.0f, see / 256.0f, 2 * state.progress() - 1.0, capture != Empty? 1 : 0);
	if(update)flt4 = coefficients * d;
	//basic move features
	if(see < 0)proce<update>(SeeMinus, w, flt4);
	if(move.is_castling())proce<update>(Castling, w, flt4);
	if(check)proce<update>(Check, w, flt4);
	proce<update>(Capture + capture, w, flt4);
	const Piece min_attacker = pos.least_valuable_attacker(turn, from);
	if(min_attacker > Knight && min_attacker < King)proce<update>(OpenEffect, w, flt4);
	if(pos.is_attacked(opponent(turn), from))proce<update>(Escape + 2 * piece + (min_attacker != Empty? 1 : 0), w, flt4);
	if(move.is_promotion())proce<update>(Promotion + move.piece_moved(), w, flt4);
	const int piece_index_to = piece_index<false>(move.piece_moved(), to, turn) * piece_index_dim;
	//piece_list
	const int piece_index_from = piece_index<false>(piece, from, turn) * piece_index_dim;
	for(int i=0;i<n_pieces;i++){
		proce<update>(To + piece_index_to + piece_list[i], w, flt4);
		proce<update>(From + piece_index_from + piece_list[i], w, flt4);
	}
	//previous move
	Move last_move = state.previous_move(0);
	Move second_last_move = state.previous_move(1);
	if(last_move != NullMove){
		const int piece_index_last = piece_index<true>(last_move.piece_moved(), last_move.to(), turn);
		proce<update>(PreviousTo + piece_index_to + piece_index_last, w, flt4);
		proce<update>(PreviousFrom + piece_index_from + piece_index_last, w, flt4);
		if(last_move.capture() != Empty && capture != Empty)proce<update>(ReCapture, w, flt4);
	}
	if(second_last_move != NullMove){
		const int piece_index_second_last = piece_index<false>(second_last_move.piece_moved(), second_last_move.to(), turn);
		proce<update>(PreviousTo + piece_index_to + piece_index_second_last, w, flt4);
		proce<update>(PreviousFrom + piece_index_from + piece_index_second_last, w, flt4);
	}
	flt4 *= coefficients;
	return flt4.sum();
}

float move_score(const State& state, Move move){
	return proce<false>(state, move, weights, 0);
}
void calc_grad(const State& state, Move move, Weights& grad, float d){
	proce<true>(state, move, grad, d);
}

void calculate_probability(int n,const sheena::Array<Move, MaxLegalMove>& moves, sheena::Array<float, MaxLegalMove>& scores, float max_score){
	float sum = 0;
	for(int i=0;i<n;i++){
		scores[i] -= max_score;
		scores[i] = std::exp(scores[i]);
		sum += scores[i];
	}
	for(int i=0;i<n;i++){
		scores[i] /= sum;
	}
}

#ifdef LEARN

static sheena::Array2d<int, friend_piece_index_dim, piece_index_dim> relation_index;
static constexpr int relation_dim = 7 * 14 * 15 * 8;
static void init_relation_table(){
	for(Piece p = PassedPawn; p != PieceDim; p++){
		for(Square sq1 = 0; sq1 < NSquare; sq1++){
			if((p == Pawn || p == PassedPawn) && (sq1 < 8 || sq1 >=56))continue;
			int index1 = piece_index<false>(p, sq1, White);
			for(Piece q = PassedPawn;q != PieceDim;q++){
				for(Square sq2 = 0; sq2 < NSquare; sq2++){
					int rel = (rank(sq1) - rank(sq2) + 7) * 8 + std::abs(file(sq1) - file(sq2));
					if((q == Pawn || q == PassedPawn) && (sq2 < 8 || sq2 >=56))continue;
					relation_index[index1][piece_index<false>(q, sq2, White)] = (p * 14 + q) * 15 * 8 + rel;
					relation_index[index1][piece_index<true>(q, sq2, White)] = (p * 14 + 7 + q) * 15 * 8 + rel;
				}
			}
		}
	}
}

static void store(){
	FILE * fp = fopen("probability.bin", "wb");
	fwrite(&weights, sizeof(Weights), 1, fp);
}

static void clear(Weights& w){
	for(int i=0;i<Weights::size();i++)w[i].clear();
}

template <bool test>
double learn_one(const State& state, Move best_move, Weights& grad){
	sheena::Array<Move, MaxLegalMove> moves;
	sheena::Array<float, MaxLegalMove> scores;
	const Position& pos = state.pos();
	//calculate probability of all moves
	int n = pos.generate_important_moves(moves, 0);
	n = pos.generate_unimportant_moves(moves, n);
	float max_score = -FLT_MAX;
	for(int i=0;i<n;i++){
		if(pos.is_suicide_move(moves[i])){
			moves[i--] = moves[--n];
			continue;
		}
		scores[i] = move_score(state, moves[i]);
		max_score = std::max(scores[i], max_score);
	}
	calculate_probability(n, moves, scores, max_score);
	//update gradient
	float best_move_score, other_max_score = 0;
	for(int i=0;i<n;i++){
		if(moves[i] == best_move){
			if(!test)calc_grad(state, moves[i], grad, scores[i] - 1);
			best_move_score = scores[i];
		}
		else{
			if(!test)calc_grad(state, moves[i], grad, scores[i]);
			other_max_score = std::max(other_max_score, scores[i]);
		}
	}
	return -std::log(best_move_score);
}

//raw weights
using LowDimWeights = sheena::Array<sheena::Float4, relation_dim * 4>;
void learn_probability(std::vector<Record>& records){
	if(records.size() < 10000){
		std::cout << "Data set is too small !" << std::endl;
		return;
	}
	init_relation_table();
	std::cout << "learning probability ..." << std::endl;
	std::vector<Sample> training_set = get_training_set(records, 1000);
	std::mt19937 mt(0);
	//training
	clear(weights);
	constexpr int batch_size = 10000;
	constexpr float learning_rate = 0.1f;
	constexpr float l1penalty = 0.001f;
	constexpr float delta = 0.000001f;
	Timer timer;
	std::unique_ptr<Weights> grad(new Weights), g2(new Weights), weights_raw(new Weights);
	clear(*weights_raw);
	clear(*grad);
	clear(*g2);
	std::unique_ptr<LowDimWeights> grad_low(new LowDimWeights), g2_low(new LowDimWeights), weights_low(new LowDimWeights);
	for(int i=0;i<LowDimWeights::size();i++){
		(*grad_low)[i].clear();
		(*g2_low)[i].clear();
		(*weights_low)[i].clear();
	}
	for(int epoch = 0;epoch < 4;epoch++){
		double loss = 0;
		int cnt = 0;
		std::shuffle(training_set.begin(), training_set.end(), mt);
		for(int i=0;i + batch_size <= training_set.size();i++){
			Sample sample = training_set[i];
			State state;
			setup_leaning_position(state, records, sample);
			loss += learn_one<false>(state, records[sample.first][sample.second], *grad);
			cnt++;
			if((i + 1) % batch_size == 0){
				//calculate grad_low
				for(int p = 0; p < friend_piece_index_dim;p++){
					for(int q = 0;q < piece_none_index;q++){
						int rel = relation_index[p][q];
						int abs = p * piece_index_dim + q;
						(*grad_low)[rel] += (*grad)[To + abs];
						(*grad_low)[relation_dim + rel] += (*grad)[From + abs];
						(*grad_low)[2 * relation_dim + rel] += (*grad)[PreviousTo + abs];
						(*grad_low)[3 * relation_dim + rel] += (*grad)[PreviousFrom + abs];
					}
				}
				//update function (AdaGrad)
				auto update = [=](sheena::Float4& w, sheena::Float4& g, sheena::Float4& g2){
					//penalty
					for(int i=0;i<4;i++){
						if(w[i] > 0)g[i] += l1penalty;
						else if(w[i] < 0)g[i] -= l1penalty;
					}
					g2 += g * g;
					w -= g * learning_rate / (g2.sqrt() + delta);
					g.clear();
				};
				//update low weights
				for(int i=0;i<LowDimWeights::size();i++){
					update((*weights_low)[i], (*grad_low)[i], (*g2_low)[i]);
				}
				//update raw weights
				for(int i=0;i<Weights::size();i++){
					update((*weights_raw)[i], (*grad)[i], (*g2)[i]);
					weights[i] = (*weights_raw)[i];
				}
				//add weights_low
				for(int p = 0; p < friend_piece_index_dim;p++){
					for(int q = 0;q < piece_none_index;q++){
						int rel = relation_index[p][q];
						int abs = p * piece_index_dim + q;
						weights[To + abs] += (*weights_low)[rel];
						weights[From + abs] += (*weights_low)[relation_dim + rel];
						weights[PreviousTo + abs] += (*weights_low)[2 * relation_dim + rel];
						weights[PreviousFrom + abs] += (*weights_low)[3 * relation_dim + rel];
					}
				}
				clear(*grad);
			}
		}
		std::cout << loss / cnt <<", "<< timer.sec() << std::endl;
	}
	//test
	double loss = 0;
	int cnt = 0;
	for(int i=0;i<1000;i++){
		const Record& record = records[i];
		State state;
		for(int j=0;j<records[i].size();j++){
			loss += learn_one<true>(state, record[j], *grad);
			cnt++;
			state.make_move(record[j]);
		}
	}
	std::cout << "test:" << loss / cnt << std::endl;
	store();
}
#endif
