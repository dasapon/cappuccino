#include "state.hpp"

extern float move_score(const State& state, Move move);

extern void learn_probability(std::vector<Record>& records);

extern void calculate_probability(int, const Array<Move, MaxLegalMove>&, Array<float, MaxLegalMove>&, float max_score);

extern void load_proabiblity();
