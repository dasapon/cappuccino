#include "probability.hpp"

enum{
	SeeMinus,
	Castling,
	Check,
	Capture,
	Dim = Capture + King,
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
	int see = pos.see(move);
	bool check = pos.is_move_check(move);
	Piece capture = move.capture();
	if(see < 0)ret += get_weight<update>(SeeMinus, w, d);
	if(move.is_castling())ret += get_weight<update>(Castling, w, d);
	if(check)ret += get_weight<update>(Check, w, d);
	ret += get_weight<update>(Capture + capture, w, d);
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
			calc_grad(state, moves[i], grad, scores[i] - 1);
			best_move_score = scores[i];
		}
		else{
			calc_grad(state, moves[i], grad, scores[i]);
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
	constexpr int batch_size = 1000;
	constexpr float learning_rate = 0.001f;
	for(int epoch = 0;epoch < 8;epoch++){
		Weights grad;
		clear(grad);
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
			accuracy += learn_one(state, record[sample.second], grad);
			cnt++;
			if((i + 1) % batch_size == 0){
				//update weights
				for(int i=0;i<Weights::size();i++){
					weights[i] -= grad[i] * learning_rate;
				}
				clear(grad);
			}
		}
		std::cout << "\n" << accuracy / cnt << std::endl;
		for(int i=0;i<Dim;i++)std::cout << weights[i] << std::endl;
	}
	//todo test
	store();
}
