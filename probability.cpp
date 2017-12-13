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
