#include "spsc_circ_fifo.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>


using namespace cliph;
using namespace std::chrono_literals;

int main()
{
	auto ts_fifo_2 = utils::spsc_circ_fifo_2<int>{};

	using elem_type = int;
	const auto kLen = 1'000'000;
	auto producer_flow = std::vector<elem_type>(kLen);
	std::for_each(std::begin(producer_flow), std::end(producer_flow), [](auto& elem)
	{
		elem = std::rand() % 256u;
	});
	//
	auto start = std::chrono::steady_clock::now();
	auto producer = std::thread{[&]
	{
		for (const auto& v : producer_flow) 
		{	
			while (true)
			{
				if (auto slot = ts_fifo_2.get_write_slot())
				{
					*slot = v;
					break;					
				}
			}
		}

		while (true)
		{
			if (auto slot = ts_fifo_2.get_write_slot())
			{
				*slot = -1;
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
	auto consumer = std::thread{[&]
	{
		auto it = std::begin(consumer_flow);
		while (true)
		{
			auto slot = ts_fifo_2.get_read_slot();
			if (*slot < 0) break;
			*it++ = *slot;
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
	std::cerr << "elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << '\n';
	assert(std::equal(std::cbegin(consumer_flow), std::cend(consumer_flow), std::cbegin(producer_flow), std::cend(producer_flow)));	

#ifdef PRINT_DEBUG
	for (auto i = 0u; i < producer_flow.size(); ++i)
	{
		std::cerr << i << " ";
		std::cerr << (int)producer_flow[i] << " ";
		std::cerr << (int)consumer_flow[i] << " ";
		std::cerr << "\n";
	}
#endif
//

#if 0
	using elem_type = int;
	const auto get_elem_type = [] { return std::rand() % 1024; };
	//
	constexpr const auto kCapacity = 64;
	constexpr const auto kElemCountInOneChunk = 1;
	//auto c_buf = cliph::utils::spsc_circ_fifo<elem_type, kElemCountInOneChunk, kCapacity>{};
	auto p1_c1_buf = cliph::utils::spsc_ts_idx_cbuf<>{};
	auto c1_c2_buf = cliph::utils::spsc_ts_idx_cbuf<>{};

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
		for (const auto& v : producer_flow) 
		{
			while (true)
			{
				if (auto w = p1_c1_buf.get_write_idx())
				{
					p1_c1_buf[*w] = v;
					p1_c1_buf.commit_write(*w);
					break;
				}
				else
				{
					producer_overrun++;
				}
			}
		}		
	}};
#if 0
	auto producer = std::thread{[&]
	{
		for (const auto& v : producer_flow) 
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
#endif
	//
	//
	//
	auto consumer1_flow = std::vector<elem_type>{};
	consumer1_flow.reserve(kLen);
	auto consumer_underrun = std::uint_fast64_t{};
	auto consumer1 = std::thread{[&]
	{
		while (producer_flow.size() != consumer1_flow.size())
		{
			auto r = c_buf.get_read_idx();
			consumer1_flow.push_back(elem_type{});
			consumer1_flow.back() = p1_c1_buf[r];
			p1_c1_buf.commit_read(r);
		}
	}};

	auto consumer2_flow = std::vector<elem_type>{};
	consumer2_flow.reserve(kLen);
	auto consumer_underrun = std::uint_fast64_t{};
	auto consumer1 = std::thread{[&]
	{
		while (producer_flow.size() != consumer_flow.size())
		{
			auto r_slot = c_buf.get_read_idx();
			consumer_flow.push_back(elem_type{});
			consumer_flow.back() = c_buf[r_slot];
			c_buf.commit_read(r_slot);
		}
	}};

#if 0
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
#endif
	//
	producer.join();
	consumer.join();
	//
	auto stop = std::chrono::steady_clock::now();	
	assert(std::equal(std::cbegin(consumer_flow), std::cend(consumer_flow)
		, std::cbegin(producer_flow), std::cend(producer_flow)));
	//
	std::cerr << "elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << '\n';
	std::cerr << "producer_overrun: " << producer_overrun << ", consumer_underrun: " << consumer_underrun << '\n';

#if 0
	for (auto i = 0; i < producer_flow.size(); ++i)
	{
		std::cerr << producer_flow[i] << ", " <<  consumer_flow[i] << '\n';
	}
#endif
#endif
}