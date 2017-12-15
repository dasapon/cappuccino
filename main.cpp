#include "position.hpp"
#include "bitboard.hpp"
#include "probability.hpp"

int main(int argc, char* argv[]){
	init_bitboard_tables();
	Position::init_hash_seed();
	load_proabiblity();
	if(argc > 1 && std::string(argv[1]) == "test"){
		bool ok = unit_test_perft();
		ok &= unit_test_see();
		std::cout << "Unit test is " << (ok? "succeed" : "failed") << std::endl;
		return 0;
	}
	else if(argc > 2 && std::string(argv[1]) == "probability"){
		std::vector<Record> records = read_pgn(argv[2], 3050);
		learn_probability(records);
		return 0;
	}
	uci_loop();
	return 0;
}
