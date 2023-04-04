#include "utils.hpp"

#include <atomic>
#include <bits/chrono.h>
#include <chrono>
#include <iostream>
#include <cassert>
#include <mutex>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>
#include <thread>


int main()
{
	using elem_type = int;
	const auto get_elem_type = [] { return std::rand() % 1024; };
	//
	constexpr const auto kCapacity = 64;
	constexpr const auto kElemCountInOneChunk = 1;
	auto c_buf = cliph::utils::spsc_circ_fifo<elem_type, kElemCountInOneChunk, kCapacity>{};
	//
	const auto kLen = 1'000'000u;
	//
	//
	//
	auto producer_flow = std::vector<elem_type>(kLen);
	std::generate(std::begin(producer_flow), std::end(producer_flow), get_elem_type);
	auto producer_overrun = std::uint_fast64_t{};
	auto start = std::chrono::steady_clock::now();
	auto producer = std::thread{[&]
	{
		for (auto v : producer_flow) 
		{
			while (true)
			{
				if (auto w_slot = c_buf.get_write_slot())
				{
					w_slot->write_into(&v, sizeof(v));
					break;
				}
				else
				{
					producer_overrun++;
				}
			}
		}
	}};
	//
	//
	//
	auto consumer_flow = std::vector<elem_type>{};
	consumer_flow.reserve(kLen);
	auto consumer_underrun = std::uint_fast64_t{};
	auto consumer = std::thread{[&]
	{
		while (producer_flow.size() != consumer_flow.size())
		{
			auto r_slot = c_buf.get_read_slot();
			consumer_flow.push_back(elem_type{});
			auto* val_ptr = std::addressof(consumer_flow.back());
			r_slot.read_from(val_ptr, sizeof(elem_type));
		}
	}};
	//
	producer.join();
	consumer.join();
	//
	auto stop = std::chrono::steady_clock::now();	
	assert(std::equal(std::cbegin(consumer_flow), std::cend(consumer_flow)
		, std::cbegin(producer_flow), std::cend(producer_flow)));
	//
	std::cerr << "elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start) << '\n';
	std::cerr << "producer_overrun: " << producer_overrun << ", consumer_underrun: " << consumer_underrun << '\n';

#if 0
	for (auto i = 0; i < producer_flow.size(); ++i)
	{
		std::cerr << producer_flow[i] << ", " <<  consumer_flow[i] << '\n';
	}
#endif
}