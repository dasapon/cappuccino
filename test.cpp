#include "position.hpp"

uint64_t perft(Position& pos, int depth){
	if(depth == 0)return 1;
	Array<Move, MaxLegalMove> moves;
	int n = pos.generate_important_moves(moves, 0);
	n = pos.generate_unimportant_moves(moves, n);
	uint64_t ret = 0;
	for(int i=0;i<n;i++){
		if(!pos.is_valid_move(moves[i]))return ret;
		if(pos.is_suicide_move(moves[i]))continue;
		Position next(pos);
		next.make_move(moves[i]);
		ret += perft(next, depth - 1);
	}
	return ret;
}

bool unit_test_perft(){
	//unit test
	//https://chessprogramming.wikispaces.com/Perft+Results
	Array<FEN, 6>fens({startpos,
		FEN({"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "0"}),
		FEN({"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8","w","-","-","0","0"}),
		FEN({"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1","w","kq","-","0","1"}),
		FEN({"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R", "w", "KQ", "-", "1", "8"}),
		FEN({"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1","w","-","-","0","10"}),});
	const Array<uint64_t, 6> results({4865609, 193690690, 674624, 15833292, 89941194, 164075551});
	bool ok = true;
	for(int i=0;i<6;i++){
		Position pos(fens[i]);
		uint64_t result = perft(pos, 5);
		ok &= results[i] == result;
		std::cout << "Pos" << i << ":" << results[i] << "," << result << std::endl;
	}
	return ok;
}
