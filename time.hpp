#include <chrono>

using namespace std::chrono;
class Timer{
	system_clock::time_point start;
public:
	Timer(){start = system_clock::now();}
	uint64_t msec()const{return duration_cast<milliseconds>(system_clock::now() - start).count();}
};

