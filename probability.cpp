#include "probability.hpp"
#include "time.hpp"
#include "learn.hpp"

enum{
	SeeMinus,
	Dim,
};

using ProbabilityWeights = sheena::Array<sheena::VFlt<4>, Dim>;

static ProbabilityWeights weights;

void load_proabiblity(){
	FILE * fp = fopen("probability.bin", "rb");
	bool fail = true;
	if(fp != NULL){
		fail = fread(&weights, sizeof(ProbabilityWeights), 1, fp) != 1;
		fclose(fp);
	}
	if(fail){
		std::cout << "probabiblity loading is failed." << std::endl;
	}
}

template<bool update>
static void proce(int idx, ProbabilityWeights& w, sheena::VFlt<4>& d){
	if(update)w[idx] += d;
	else d += w[idx];
}

template<bool update, Player turn>
static float proce(const State& state, Move move, ProbabilityWeights& w, float d){
	const Position& pos = state.pos();
	const int see = pos.see(move);
	sheena::VFlt<4> vflt;
	if(update){
		vflt = d;
	}
	else{
		vflt.clear();
	}
	if(see < 0)proce<update>(SeeMinus, w, vflt);
	return vflt[0];
}

template<Player turn>
float move_score(const State& state, Move move){
	return proce<false, turn>(state, move, weights, 0);
}

template float move_score<White>(const State& state, Move move);
template float move_score<Black>(const State& state, Move move);
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
static constexpr size_t batch_size = 10000;
static void clear_weights(ProbabilityWeights& pw){
	for(int i=0;i<Dim;i++){
		pw[i].clear();
	}
}

template<Player turn>
static void calc_grad(const State& state, Move move, ProbabilityWeights& grad, float d){
	proce<true, turn>(state, move, grad, d);
}

static void update_weight(sheena::ArrayAlloc<ProbabilityWeights>& grads, float learning_rate){
#pragma omp parallel for
	for(int i=0;i<Dim;i++){
		for(int j=1;j<learning_threads;j++){
			grads[0][i] += grads[j][i];
			grads[j][i].clear();
		}
		weights[i] -= grads[0][i] * learning_rate;
		//momentum
		grads[0][i] *= 0.95;
	}
}

static void store(){
	FILE * fp = fopen("probability.bin", "wb");
	fwrite(&weights, sizeof(ProbabilityWeights), 1, fp);
}

template<Player turn>
static void learn_one_pos(const State& state, Move bestmove, ProbabilityWeights& grad, ClassificationStatistics& statistics){
	sheena::Array<Move, MaxLegalMove> moves;
	sheena::Array<float, MaxLegalMove> scores;
	const Position& pos = state.pos();
	//calculate probability of all moves
	int n = pos.generate_important_moves(moves, 0);
	n = pos.generate_unimportant_moves(moves, n);
	float best_move_score = 0, other_max_score = -FLT_MAX;
	for(int i=0;i<n;i++){
		if(pos.is_suicide_move(moves[i])){
			moves[i--] = moves[--n];
			continue;
		}
		scores[i] = move_score<turn>(state, moves[i]);
		if(moves[i] == bestmove){
			best_move_score = scores[i];
		}
		else{
			other_max_score = std::max(other_max_score, scores[i]);
		}
	}
	calculate_probability(n, moves, scores, std::max(other_max_score, best_move_score));
	//update gradient
	for(int i=0;i<n;i++){
		if(moves[i] == bestmove){
			calc_grad<turn>(state, moves[i], grad, scores[i] - 1);
			statistics.update(best_move_score > other_max_score, -std::log(scores[i]));
		}
		else{
			calc_grad<turn>(state, moves[i], grad, scores[i]);
		}
	}
}

extern void learn_probability(const std::vector<Record>& records){
	sheena::ArrayAlloc<ProbabilityWeights> grads(learning_threads);
	sheena::ArrayAlloc<State> states(learning_threads);
	for(auto& g : grads){
		clear_weights(g);
	}
	std::vector<Sample> samples = get_training_set(records);
	omp_set_num_threads(learning_threads);
	std::mt19937 mt(0);
	sheena::Stopwatch stopwatch;
	for(int rep = 0;rep < 8;rep++){
		float learning_rate = rep < 4? 0.01 : 0.001;
		sheena::ArrayAlloc<ClassificationStatistics> statistics(learning_threads);
		std::shuffle(samples.begin(), samples.end(), mt);
		for(int i=0;i < samples.size() / batch_size;i++){
#pragma omp parallel for
			for(int j=i * batch_size;j < (i + 1) * batch_size;j++){
				size_t thread_id = omp_get_thread_num();
				setup_learning_position(states[thread_id], records, samples[j]);
				if(states[thread_id].pos().turn_player() == White)learn_one_pos<White>(states[thread_id], records[samples[j].first][samples[j].second], grads[thread_id], statistics[thread_id]);
				else learn_one_pos<Black>(states[thread_id], records[samples[j].first][samples[j].second], grads[thread_id], statistics[thread_id]);
			}
			update_weight(grads, learning_rate);
		}
		for(int i=1;i<learning_threads;i++)statistics[0] += statistics[i];
		std::cout << statistics[0].accuracy() << "," << statistics[0].average_loss() << "," << stopwatch.sec() << std::endl;
	}
	store();
}
#endif