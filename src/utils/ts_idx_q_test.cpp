#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
//
#include "ts_idx_q.hpp"

using namespace cliph;
using namespace std::chrono_literals;

int main()
{
	using elem_type = int;

	auto wrapped = std::array<int, 64u>{};
	auto q = utils::ts_idx_q{wrapped};

	using slot_t = typename decltype(q)::slot_type;
	auto slot_q = utils::ts_cbuf<slot_t>{};

	const auto kLen = 1'000'000;
	auto producer_flow = std::vector<elem_type>(kLen);
	std::for_each(std::begin(producer_flow), std::end(producer_flow), [](auto& elem)
	{
		elem = std::rand() % 256u;
	});
	//
	auto start = std::chrono::steady_clock::now();
	auto producer_overrun = 0ull;
	auto producer = std::thread{[&]
	{
		for (const auto& v : producer_flow) 
		{
			for (auto slot = q.get();; slot = q.get())
			{
				if (slot)
				{
					*slot = v;
					slot_q.put(std::move(slot));
					break;
				}
				++producer_overrun;
			}
		}

		for (auto slot = q.get();; slot = q.get())
		{
			if (slot)
			{
				*slot = -1;
				slot_q.put(std::move(slot));
				break;
			}
		}
		std::cerr << "producer done\n";
	}};
	if (auto rc = ::pthread_setname_np(producer.native_handle(), "PRODUCER"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
	//
	auto consumer_flow = std::vector<elem_type>(kLen);
	auto consumer_underrun = 0ull;
	auto consumer = std::thread{[&]
	{
		auto it = std::begin(consumer_flow);
		while (true)
		{
			auto slot = slot_t{};
#if 0
			slot_q.get(slot);
			if (*slot < 0) { break; }
			*it++ = *slot;
#endif
#if 1
			if (slot_q.try_get(slot))
			{
				if (*slot < 0) { break; }
				*it++ = *slot;
			}
			else
			{
				++consumer_underrun;
			}
#endif
		}
		std::cerr << "consumer done\n";
	}};
	if (auto rc = ::pthread_setname_np(consumer.native_handle(), "CONSUMER"))
	{
		throw std::runtime_error{"pthread_setname_np"};
	}
	//
	producer.join();
	consumer.join();
	//
	auto stop = std::chrono::steady_clock::now();	
	//
#if 0
	for (auto i = 0u; i < producer_flow.size(); ++i)
	{
		std::cerr << i << " ";
		std::cerr << (int)producer_flow[i] << " ";
		std::cerr << (int)consumer_flow[i] << " ";
		std::cerr << "\n";
	}
#endif
	if (!std::equal(std::cbegin(consumer_flow), std::cend(consumer_flow), std::cbegin(producer_flow), std::cend(producer_flow)))
	{
		assert(false);
	}
	std::cerr << "elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << '\n';
	std::cerr << "producer_overrun: " << producer_overrun << ", consumer_underrun: " << consumer_underrun << '\n';
}