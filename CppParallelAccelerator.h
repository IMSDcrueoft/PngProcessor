#pragma once
#ifndef CPPPARALLELACCELERATOR
#define CPPPARALLELACCELERATOR

#include <cstdint>
#include <vector>
#include <memory>
#include <thread>
#include <queue>

#define DEBUG false

class CppParallelAccelerator
{

public:
	CppParallelAccelerator();
	CppParallelAccelerator(const uint16_t& numThreads);

	void SetNumThreads(const uint16_t& numThreads);
	uint16_t GetNumThreads();

	template<typename Event, typename startParameterQueue>
	void Run(Event& e, startParameterQueue& queue);

	template<typename Event, typename parameterType>
	void Run(Event& e, std::queue<parameterType>& queue);

	void Join();

protected:
	std::vector<std::unique_ptr<std::thread>> allThreads;
};


inline CppParallelAccelerator::CppParallelAccelerator()
{
#if !DEBUG
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;
#else
	uint32_t numberOfExecutionThreads = 1;
#endif

	allThreads.resize(numberOfExecutionThreads);
}

inline CppParallelAccelerator::CppParallelAccelerator(const uint16_t& numThreads)
{
	allThreads.resize(numThreads);
}

inline void CppParallelAccelerator::SetNumThreads(const uint16_t& numThreads)
{
	Join();
	allThreads.resize(numThreads);
}

inline uint16_t CppParallelAccelerator::GetNumThreads()
{
	return allThreads.size();
}
inline void CppParallelAccelerator::Join()
{
	for (auto& thread : allThreads)
	{
		if (thread->joinable())
			thread->join();
	}
}

template<typename Event, typename startParameterQueue>
inline void CppParallelAccelerator::Run(Event& e, startParameterQueue& queue)
{
	for (auto& thread : allThreads)
	{
		if (queue.empty())
			break;
		else
		{
			thread = std::make_unique<std::thread>(e, queue.front());
			queue.pop();
		}
	}
}

template<typename Event, typename parameterType>
inline void CppParallelAccelerator::Run(Event& e, std::queue<parameterType>& queue)
{
	CppParallelAccelerator::Run<Event,std::queue<parameterType>>(e, queue);
}
#endif // !CPPPARALLELACCELERATOR