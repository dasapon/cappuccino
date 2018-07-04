#pragma once

#ifdef LEARN
#include "cappuccino.hpp"

using Sample = std::pair<int, int>;

constexpr size_t learning_threads = 10;

extern void learn_eval(std::vector<Record>& records);
extern void learn_probability(const std::vector<Record>& records);
inline std::vector<Sample> get_training_set(const std::vector<Record>& records){
	std::vector<Sample> training_set;
	for(int i=0;i<records.size();i++){
		for(int j=0;j<records[i].size();j++){
			training_set.push_back(Sample(i, j));
		}
	}
	return training_set;
}

inline void setup_learning_position(State& state, const std::vector<Record>& records, Sample sample){
	const Record& record = records[sample.first];
	state.unmake_all_move();
	for(int ply = 0;ply < sample.second;ply++){
		state.make_move(record[ply]);
	}
}

class ClassificationStatistics{
	int top1, count;
	double loss;
public:
	ClassificationStatistics():top1(0), count(0), loss(0){}
	double accuracy()const{
		return double(top1) / count;
	}
	double average_loss()const{
		return loss / count;
	}
	void update(bool hit, double ll){
		count++;
		if(hit)top1++;
		loss += ll;
	}
	void operator+=(const ClassificationStatistics& rhs){
		top1 += rhs.top1;
		count += rhs.count;
		loss += rhs.loss;
	}
};

#endif
