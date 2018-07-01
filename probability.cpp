#include "probability.hpp"
#include "time.hpp"
#include "learn.hpp"

enum{
	See,
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
	return 0;
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
