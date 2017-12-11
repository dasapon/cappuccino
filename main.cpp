#include "position.hpp"
#include "bitboard.hpp"

int main(int argc, char* argv[]){
	init_bitboard_tables();
	Position::init_hash_seed();
	if(argc > 1 && std::string(argv[1]) == "test"){
		bool ok = unit_test_perft();
		ok &= unit_test_see();
		std::cout << "Unit test is " << (ok? "succeed" : "failed") << std::endl;
		return 0;
	}
	uci_loop();
	return 0;
}
