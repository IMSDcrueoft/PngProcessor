#pragma once
#ifndef CLOCKTIMER
#define CLOCKTIMER

#include<ctime>
class clockTimer
{
	std::clock_t start = 0;
	std::clock_t end = 0;
	double time = 0.0;

public:
	inline void TimerStart()
	{
		start = clock();
	}

	inline void TimerStop()
	{
		end = clock();
	}

	double& getTime()
	{
		time = (static_cast<double>(end) - start) / CLOCKS_PER_SEC;
		return time;
	}
};
#endif // !CLOCKTIMER

