#pragma once
#include "state.hpp"

template<Player turn>
extern float move_score(const State& state, Move move);

extern void calculate_probability(int, const sheena::Array<Move, MaxLegalMove>&, sheena::Array<float, MaxLegalMove>&, float max_score);

extern void load_proabiblity();
