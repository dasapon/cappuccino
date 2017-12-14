#include "probability.hpp"

enum{
	See,
	SeeMinus,
	Castling,
	Check,
	Capture,
	To = Capture + King,
	Dim = To + friend_piece_index_dim * piece_index_dim,
};

using Weights = Array<float, Dim>;

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
static void store(){
	FILE * fp = fopen("probability.bin", "wb");
	fwrite(&weights, sizeof(Weights), 1, fp);
}

static void clear(Weights& w){
	for(int i=0;i<Weights::size();i++)w[i] = 0;
}

template<bool update>
static float get_weight(int idx, Weights& w, float d){
	if(update)w[idx] += d;
	return w[idx];
}

template<bool update>
static float proce(const State& state, Move move, Weights& w, float d){
	float ret = 0;
	const Position& pos = state.pos();
	int n_pieces;
	const Array<int, 32>& piece_list = state.get_piece_list(&n_pieces);
	int see = pos.see(move);
	bool check = pos.is_move_check(move);
	Piece piece = move.piece();
	Piece capture = move.capture();
	const Player turn = pos.turn_player();
	Square to = move.to();
	if(update)w[See] += d * see / 256;
	else ret += w[See] * see / 256;
	if(see < 0)ret += get_weight<update>(SeeMinus, w, d);
	if(move.is_castling())ret += get_weight<update>(Castling, w, d);
	if(check)ret += get_weight<update>(Check, w, d);
	ret += get_weight<update>(Capture + capture, w, d);
	const int piece_index_to = piece_index<false>(piece, to, turn) * piece_index_dim;
	for(int i=0;i<n_pieces;i++){
		ret += get_weight<update>(To + piece_index_to + piece_list[i], w, d);
	}
	return ret;
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
using Sample = std::pair<int, int>;
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
	return best_move_score > other_max_score ? 1: 0;
}

void learn_probability(std::vector<Record>& records){
	if(records.size() < 10000){
		std::cout << "Data set is too small !" << std::endl;
		return;
	}
	std::cout << "learning probability ..." << std::endl;
	std::cout << "Dim = " << Dim << std::endl;
	std::vector<Sample> training_set;
	std::mt19937 mt(0);
	std::shuffle(records.begin(), records.end(), mt);
	for(int i=1000;i<records.size();i++){
		for(int j=0;j<records[i].size() && j < learning_ply_limit;j++){
			training_set.push_back(Sample(i, j));
		}
	}
	//training
	clear(weights);
	constexpr int batch_size = 2000;
	constexpr float learning_rate = 0.1f;
	constexpr float delta = 0.000001f;
	Weights grad, g2;
	clear(grad);
	clear(g2);
	for(int epoch = 0;epoch < 4;epoch++){
		double accuracy = 0;
		int cnt = 0;
		std::shuffle(training_set.begin(), training_set.end(), mt);
		for(int i=0;i + batch_size <= training_set.size();i++){
			Sample sample = training_set[i];
			State state;
			const Record& record = records[sample.first];
			for(int ply = 0;ply < sample.second;ply++){
				state.make_move(record[ply]);
			}
			accuracy += learn_one<false>(state, record[sample.second], grad);
			cnt++;
			if((i + 1) % batch_size == 0){
				//update weights
				for(int i=0;i<Weights::size();i++){
					g2[i] += grad[i] * grad[i];
					weights[i] -= grad[i] * learning_rate / (std::sqrt(g2[i]) + delta);
				}
				clear(grad);
			}
		}
		std::cout << accuracy / cnt << std::endl;
	}
	//test
	double accuracy = 0;
	int cnt = 0;
	for(int i=0;i<1000;i++){
		const Record& record = records[i];
		State state;
		for(int j=0;j<records[i].size();j++){
			accuracy += learn_one<true>(state, record[j], grad);
			cnt++;
			state.make_move(record[j]);
		}
	}
	std::cout << "test:" << accuracy / cnt << std::endl;
	store();
}
