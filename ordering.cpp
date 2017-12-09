#include "search.hpp"

void MoveOrderer::insertion_sort(int start, int end){
	int i, j;
	float s;
	Move m;
	for (i = start + 1; i < end; i++) {
		s = scores[i];
		m = moves[i];
		if (s > scores[i - 1]) {
			j = i;
			do {
				scores[j] = scores[j - 1];
				moves[j] = moves[j - 1];
				j--;
			} while (j  > start && s > scores[j - 1]);
			scores[j] = s;
			moves[j] = m;
		}
	}
}
