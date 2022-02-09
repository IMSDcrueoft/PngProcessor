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

public:
	template<typename Index_type, typename Function>
	static void parallel_for(Index_type First, const Index_type Last, Function&& Func);

	template<typename Index_type, typename Function>
	static void parallel_for(Index_type First, const Index_type Last, const Index_type Step, Function&& Func);

protected:
	std::vector<std::unique_ptr<std::thread>> allThreads;
};


inline CppParallelAccelerator::CppParallelAccelerator()
{
#if !DEBUG
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 4) ? numberOfExecutionThreads : 4;
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

template<typename Index_type, typename Function>
inline void CppParallelAccelerator::parallel_for(Index_type First, const Index_type Last, Function&& Func)
{
	CppParallelAccelerator::parallel_for(First, Last,static_cast<Index_type>(1),std::forward<Function>(Func));
}

template<typename Index_type, typename Function>
inline void CppParallelAccelerator::parallel_for(Index_type First, const Index_type Last, const Index_type Step, Function&& Func)
{
	CppParallelAccelerator accelerator;
	std::queue<Index_type> param;

	while (First < Last)
	{
		for (uint16_t i = 0; i < accelerator.GetNumThreads(); ++i)
		{
			if ((First + i * Step) < Last)
				param.push((First + i * Step));
			else
				break;
		}

		accelerator.Run(Func, param);
		accelerator.Join();

		First += Step * accelerator.GetNumThreads();
	}
}
#endif // !CPPPARALLELACCELERATOR