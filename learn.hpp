#pragma once

#ifdef LEARN
#include "cappuccino.hpp"

using Sample = std::pair<int, int>;

extern void learn_eval(std::vector<Record>& records);
extern void learn_probability(std::vector<Record>& records);
inline std::vector<Sample> get_training_set(std::vector<Record>& records, int test_size){
	std::mt19937 mt(0);
	std::vector<Sample> training_set;
	std::shuffle(records.begin(), records.end(), mt);
	for(int i=1000;i<records.size();i++){
		for(int j=0;j<records[i].size() && j < learning_ply_limit;j++){
			training_set.push_back(Sample(i, j));
		}
	}
	return training_set;
}
inline void setup_leaning_position(State& state, const std::vector<Record>& records, Sample sample){
	const Record& record = records[sample.first];
	state.unmake_all_move();
	for(int ply = 0;ply < sample.second;ply++){
		state.make_move(record[ply]);
	}
}
#endif
