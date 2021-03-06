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
	static constexpr uint64_t upper_bound = 0x10, lower_bound = 0x20, exact = 0x30;
	static constexpr uint64_t key_mask = 0xffffffff00000000ULL;
public:
	HashEntry(){}
	HashEntry(uint64_t key, Move move, int depth, int value, int alpha, int beta, uint64_t generation){
		word1 = (key & key_mask)
			| move.to_int();
		word2 = (static_cast<uint64_t>(value + ValueINF) << 48) | (static_cast<uint64_t>(depth + 512) << 32)
			| get_flag(alpha, beta, value) | generation;
	}
	bool hit(uint64_t key)const{return (key & key_mask) == (word1 & key_mask);}
	Move move()const{
		return Move(static_cast<int>(word1 & ~key_mask));
	}
	int depth()const{
		return static_cast<int>((word2 >> 32) & 0xffff) - 512;
	}
	bool hash_cut(int& value, int alpha, int beta, int depth){
		int d = this->depth();
		if(d >= depth){
			int v = static_cast<int>(word2 >> 48) - ValueINF;
			uint64_t flg = word2 & exact;
			if((flg & upper_bound) != 0 && v <= alpha){
				value = v;
				return true;
			}
			else if((flg & lower_bound) != 0 && v >= beta){
				value = v;
				return true;
			}
			else if(flg == exact){
				value = v;
				return true;
			}
		}
		return false;
	}
	uint64_t generation()const{
		return word2 & 0xf;
	}
	void clear(){
		word1 = word2 = 0;
	}
};

class HashTable{
	sheena::ArrayAlloc<HashEntry> table;
	size_t mask;
	uint64_t generation;
public:
	void set_size(size_t mb){
		while(mb & (mb - 1))mb &= mb - 1;
		if(mb == 0)std::cout << "Invalid set. hash size must not be 0." << std::endl;
		mask = mb * (1 << 20) / sizeof(HashEntry) - 1;
		table.resize(mask + 1);
	}
	void clear(){
		for(int i=0;i<table.size();i++)table[i].clear();
		generation = 1;
	}
	void new_gen(){generation = (generation + 1) & 0xf;}
	HashTable():generation(0){
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
		HashEntry entry(key, move, depth, value, alpha, beta, generation);
		const uint64_t idx = key & mask;
		if(table[idx].generation() != generation
			|| table[idx].depth() <= depth)table[idx] = entry;
	}
};
