#include "bitboard.hpp"
#include "probability.hpp"
#include "learn.hpp"
#include "evaluate.hpp"

int main(int argc, char* argv[]){
	init_bitboard_tables();
	Position::init_hash_seed();
	load_proabiblity();
	load_eval();
	if(argc > 1 && std::string(argv[1]) == "unit_test"){
		bool ok = unit_test_perft();
		ok &= unit_test_see();
		std::cout << "Unit test is " << (ok? "succeed" : "failed") << std::endl;
		return 0;
	}
	if(argc > 1 && std::string(argv[1]) == "search_test"){
		search_test();
		return 0;
	}
#ifdef LEARN
	else if(argc > 2 && std::string(argv[1]) == "probability"){
		//std::vector<Record> records;// = read_pgn(argv[2], 2400);
		//learn_probability(records);
	}
	else if(argc > 2 && std::string(argv[1]) == "eval"){
		//std::vector<Record> records = read_pgn(argv[2], 3100);
		//learn_eval(records);
	}
#else
	uci_loop();
#endif
	return 0;
}
