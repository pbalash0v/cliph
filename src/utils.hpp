#ifndef circular_hpp
#define circular_hpp

#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <array>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <iostream>
#include <optional>

namespace cliph::utils
{


template<typename T, auto one_chunk_sz = 2048u, auto num_of_chunks = 64u>
class spsc_circ_fifo final
{

template <typename Q>
class slot
{
public:
	void write_into(const void* start, std::size_t len)
	{
		if (len > one_chunk_sz) { throw std::runtime_error {"Length is too big"}; }

		std::memcpy(std::get<0>(m_owner->m_buf[m_num]).data(), start, len);
		std::get<1>(m_owner->m_buf[m_num]) = len;

		m_owner->m_write_pos = (m_owner->m_write_pos + 1) % num_of_chunks;
		m_owner->m_cond.notify_one();
	}

	std::size_t read_from(void* start, std::size_t len)
	{
		const auto& sz = std::get<1>(m_owner->m_buf[m_num]);

		if (len < sz) { throw std::runtime_error {"Length is too big"}; }

		std::memcpy(start, std::get<0>(m_owner->m_buf[m_num]).data(), sz);

		auto l_g = std::lock_guard{m_owner->m_mtx};
		m_owner->m_read_pos = (m_owner->m_read_pos + 1) % num_of_chunks;

		return sz;
	}

private:
	slot(Q* q, std::uint_fast64_t num)
		: m_owner{q}
		, m_num{num}
	{}
	Q* m_owner;
	std::uint_fast64_t m_num{};
	template<typename, auto, auto> friend class spsc_circ_fifo;
};

//
public:	
	using chunk_type = std::tuple<std::array<T, one_chunk_sz>, unsigned>;
	using q_type = spsc_circ_fifo<T, one_chunk_sz, num_of_chunks>;

public:	
	[[nodiscard]] std::optional<slot<q_type>> get_write_slot()
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto l = std::unique_lock{m_mtx, std::adopt_lock};
		//
		if (((m_write_pos + 1) % num_of_chunks) == m_read_pos)
		{
			m_cond.notify_one(); // this needed ?
			return std::nullopt;
		}
		else
		{
			return slot<q_type>{this, m_write_pos};
		}
	}

	[[nodiscard]] slot<q_type> get_read_slot()
	{
		auto l = std::unique_lock{m_mtx};
		m_cond.wait(l, [&] { return m_write_pos != m_read_pos; });
		//
		return {this, m_read_pos};
	}

private:
	std::mutex m_mtx;
	std::condition_variable m_cond;
	std::array<chunk_type, one_chunk_sz> m_buf;
	alignas(64) std::uint_fast64_t m_write_pos{};
	alignas(64) std::uint_fast64_t m_read_pos{};
};


template<typename T, auto chunk_sz = 2048u, auto capacity = 64u>
class th_safe_circ_buf final
{
public:	
	using chunk_type = std::tuple<std::array<T, chunk_sz>, unsigned>;

public:	
	[[nodiscard]] bool put(const void* start, std::size_t len)
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto l = std::unique_lock{m_mtx, std::adopt_lock};
		//
		if (((m_write_pos + 1) % capacity) == m_read_pos)
		{
			return false;
		}
		else
		{
			if (len > chunk_sz)
			{
				throw std::runtime_error {"Length is too big"};
			}
			std::memcpy(std::get<0>(m_buf[m_write_pos]).data(), start, len);
			std::get<1>(m_buf[m_write_pos]) = len;
			m_write_pos = (m_write_pos + 1) % capacity;
			l.unlock();
			m_cond.notify_one();
			return true;
		}
	}

	[[nodiscard]] std::size_t get(void* start)
	{
		auto l = std::unique_lock{m_mtx};
		m_cond.wait(l, [&] { return m_write_pos != m_read_pos; });
		//
		if (start == nullptr) { return 0; }

		const auto& sz = std::get<1>(m_buf[m_read_pos]);
		std::memcpy(start, std::get<0>(m_buf[m_read_pos]).data(), sz);
		m_read_pos = (m_read_pos + 1) % capacity;
		return sz;
	}

private:
	std::mutex m_mtx;
	std::condition_variable m_cond;
	std::array<chunk_type, capacity> m_buf;
	alignas(64) std::uint_fast64_t m_write_pos{};
	alignas(64) std::uint_fast64_t m_read_pos{};
};
using audio_circ_buf = th_safe_circ_buf<std::int16_t>;


//TODO: implement proper incapsulation in class
struct thread_safe_array
{
	inline static constexpr const auto k_capacity{2048u};
	std::size_t m_size{};
	std::array<std::uint8_t, k_capacity> m_buffer;
	bool m_is_empty{true};
	//
	std::mutex m_mtx;
	std::condition_variable m_cond;
	//
	void mark_empty_locked();
	void mark_filled_locked(std::size_t);
	//
	void lock();
	void unlock();
	auto try_lock() noexcept;
};

inline void thread_safe_array::mark_empty_locked()
{
	m_is_empty = true;
	m_size = 0;
}
inline void thread_safe_array::mark_filled_locked(std::size_t sz)
{
	m_is_empty = false;
	m_size = sz;
}
//
inline void thread_safe_array::lock()
{
	m_mtx.lock();
}
inline void thread_safe_array::unlock()
{
	m_mtx.unlock();
}
inline auto thread_safe_array::try_lock() noexcept
{
	return m_mtx.try_lock();
}

} //namespace cliph::utils

#endif // circular_hpp