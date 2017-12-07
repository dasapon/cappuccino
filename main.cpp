#include "position.hpp"
#include "bitboard.hpp"

int main(int argc, char* argv[]){
	init_bitboard_tables();
	Position::init_hash_seed();
	if(argc > 1 && std::string(argv[1]) == "test"){
		unit_test_perft();
	}
	uci_loop();
	return 0;
}
