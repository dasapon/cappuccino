class HashEntry{
	uint64_t word1, word2;
	/*
	 * word1
	 * 00-31	key
	 * 32-63	move
	 * 
	 * word2
	 * 00-15	value
	 * 16-31	depth
	 * 58-59	flag
	 * 60-63	generation
	 */
	uint64_t get_flag(int alpha, int beta, int value){
		if(value <= alpha)return upper_bound;
		else if(value >= beta)return lower_bound;
		else return exact;
	}
public:
	static constexpr uint64_t upper_bound = 0x10, lower_bound = 0x20, exact = 0x30;
	HashEntry(){}
	HashEntry(uint64_t key, Move move, int depth, int value, int alpha, int beta, uint64_t generation){
		word1 = (key & 0xffffffff00000000)
			| move.to_int();
		word2 = (static_cast<uint64_t>(value + ValueINF) << 48) | (static_cast<uint64_t>(depth + 128) << 32)
			| get_flag(alpha, beta, value) | generation;
	}
	bool hit(uint64_t key)const{return (key & 0xffffffff00000000) == (word1 & 0xffffffff00000000);}
	Move move()const{
		return Move(static_cast<int>(word1 & 0xffffffff));
	}
};

class HashTable{
	HashEntry * table;
	size_t mask;
	uint64_t generation;
public:
	void set_size(size_t mb){
		while(mb & (mb - 1))mb &= mb - 1;
		if(mb == 0)std::cout << "Invalid set. hash size must not be 0." << std::endl;
		if(table != nullptr)delete[] table;
		mask = mb * (1 << 20) / sizeof(HashEntry) - 1;
		table = new HashEntry[mask + 1];
	}
	void clear(){
		memset(table, 0, sizeof(HashEntry) * (mask + 1));
		generation = 0;
	}
	void new_gen(){generation = (generation + 1) & 0xf;}
	HashTable():table(nullptr),generation(0){
		set_size(32);
		clear();
	}
	bool probe(const Position& pos, HashEntry& entry)const{
		uint64_t key = pos.key();
		entry = table[key & mask];
		return entry.hit(key);
	}
	void store(const Position& pos, Move move, int depth, int value, int alpha, int beta){
		uint64_t key = pos.key();
		table[key & mask] = HashEntry(key, move, depth, value, alpha, beta, generation);
	}
};
