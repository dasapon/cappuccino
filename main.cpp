#include "position.hpp"
#include "bitboard.hpp"

int main(void){
	init_bitboard_tables();
	Position::init_hash_seed();
	uci_loop();
	return 0;
}
