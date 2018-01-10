#include "simd.hpp"
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
	To = Escape + 2 * PieceDim,
	From = To + friend_piece_index_dim * piece_index_dim,
	PreviousTo = From + friend_piece_index_dim * piece_index_dim,
	PreviousFrom = PreviousTo + friend_piece_index_dim * piece_index_dim,
	Dim = PreviousFrom + friend_piece_index_dim * piece_index_dim,
};

using Weights = Array<Float4, Dim>;

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
static void proce(int idx, Weights& w, Float4& d){
	if(update)w[idx] += d;
	else d += w[idx];
}

template<bool update>
static float proce(const State& state, Move move, Weights& w, float d){
	Float4 flt4 = 0;
	const Position& pos = state.pos();
	int n_pieces;
	const Array<int, 32>& piece_list = state.get_piece_list(&n_pieces);
	int see = pos.see(move);
	bool check = pos.is_move_check(move);
	Piece piece = move.piece();
	Piece capture = move.capture();
	const Player turn = pos.turn_player();
	Square to = move.to();
	Square from = move.from();
	const float progress = (n_pieces - 2.0f) / 30.0f;
	Float4 coefficients(1.0f, see / 256.0f, 2 * progress - 1.0, capture != Empty? 1 : 0);
	if(update)flt4 = coefficients * d;
	//basic move features
	if(see < 0)proce<update>(SeeMinus, w, flt4);
	if(move.is_castling())proce<update>(Castling, w, flt4);
	if(check)proce<update>(Check, w, flt4);
	proce<update>(Capture + capture, w, flt4);
	if(pos.is_attacked(opponent(turn), from))proce<update>(Escape + 2 * piece + (pos.is_attacked(turn, from)? 1 : 0), w, flt4);
	if(move.is_promotion())proce<update>(Promotion + move.piece_moved(), w, flt4);
	//to and piece_list
	const int piece_index_to = piece_index<false>(move.piece_moved(), to, turn) * piece_index_dim;
	for(int i=0;i<n_pieces;i++){
		proce<update>(To + piece_index_to + piece_list[i], w, flt4);
	}
	//from and piece_list
	const int piece_index_from = piece_index<false>(piece, from, turn) * piece_index_dim;
	for(int i=0;i<n_pieces;i++){
		proce<update>(From + piece_index_from + piece_list[i], w, flt4);
	}
	//previous move
	Move last_move = state.previous_move(0);
	Move second_last_move = state.previous_move(1);
	if(last_move != NullMove){
		const int piece_index_last = piece_index<true>(last_move.piece_moved(), last_move.to(), turn);
		proce<update>(PreviousTo + piece_index_to + piece_index_last, w, flt4);
		proce<update>(PreviousFrom + piece_index_from + piece_index_last, w, flt4);
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

void calculate_probability(int n,const Array<Move, MaxLegalMove>& moves, Array<float, MaxLegalMove>& scores, float max_score){
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

static void store(){
	FILE * fp = fopen("probability.bin", "wb");
	fwrite(&weights, sizeof(Weights), 1, fp);
}

static void clear(Weights& w){
	for(int i=0;i<Weights::size();i++)w[i].clear();
}

template <bool test>
double learn_one(const State& state, Move best_move, Weights& grad){
	Array<Move, MaxLegalMove> moves;
	Array<float, MaxLegalMove> scores;
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

void learn_probability(std::vector<Record>& records){
	if(records.size() < 10000){
		std::cout << "Data set is too small !" << std::endl;
		return;
	}
	std::cout << "learning probability ..." << std::endl;
	std::vector<Sample> training_set = get_training_set(records, 1000);
	std::mt19937 mt(0);
	//training
	clear(weights);
	constexpr int batch_size = 10000;
	constexpr float learning_rate = 0.1f;
	constexpr float delta = 0.000001f;
	Timer timer;
	std::unique_ptr<Weights> grad(new Weights), g2(new Weights);
	clear(*grad);
	clear(*g2);
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
				//update weights
				for(int i=0;i<Weights::size();i++){
					Float4 g = (*grad)[i];
					(*g2)[i] += g * g;
					weights[i] -= g * learning_rate / ((*g2)[i].sqrt() + delta);
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
