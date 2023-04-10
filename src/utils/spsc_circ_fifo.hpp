#ifndef spsc_circ_fifo_hpp
#define spsc_circ_fifo_hpp

#include "common_std.hpp"
#include <cstdio>
#include <mutex>
#include <utility>


namespace cliph::utils
{

template<typename T, auto cap = 64u>
class spsc_circ_fifo final
{
public:	
	using q_type = spsc_circ_fifo<T, cap>;

template <typename Q>
class slot
{
public:
	T& operator*()
	{
		return m_owner->m_buf[m_idx];
	}

	slot(const slot& oth) = delete;
	slot& operator=(const slot&) = delete;

	slot(slot&& oth)
		: m_owner{std::exchange(oth.m_owner, nullptr)}
		, m_is_write{std::exchange(oth.m_is_write, false)}
		, m_idx{std::exchange(oth.m_idx, 0)}
	{
	}

	slot& operator=(slot&& oth) noexcept
	{
		if (&oth == this) { return *this; }
		m_owner = std::exchange(oth.m_owner, nullptr);
		m_is_write = std::exchange(oth.m_is_write, false);
		m_idx = std::exchange(oth.m_idx, 0);
		return *this;
	}

	explicit operator bool() const noexcept { return m_owner != nullptr; }

	~slot()
	{
		if (!*this) { return; }

		auto lk = std::unique_lock{m_owner->m_mtx};

		if (m_is_write)
		{
			m_owner->m_write_pos = (m_owner->m_write_pos + 1) % cap;
			lk.unlock();			
			m_owner->m_cond.notify_one();
		}
		else
		{
			m_owner->m_read_pos = (m_owner->m_read_pos + 1) % cap;
		}
	}

private:
	slot(Q* q, std::uint_fast64_t idx, bool is_write)
		: m_owner{q}
		, m_is_write{is_write}
		, m_idx{idx}
	{}
	//
	Q* m_owner{};
	bool m_is_write{false};
	std::uint_fast64_t m_idx{};
	//
	template<typename, auto> friend class spsc_circ_fifo;
};

public:	
	[[nodiscard]] slot<q_type> get_write_slot()
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto lk = std::lock_guard{m_mtx, std::adopt_lock};
		//
		if (((m_write_pos + 1) % cap) == m_read_pos)
		{
			return {nullptr, 0, /*is_write*/ true};
		}

		return {this, m_write_pos, /*is_write*/ true};
	}

	[[nodiscard]] slot<q_type> get_read_slot()
	{
		auto l = std::unique_lock{m_mtx};
		m_cond.wait(l, [&] { return m_write_pos != m_read_pos; });
		//
		return {this, m_read_pos, /*is_write*/ false};
	}

private:
	std::mutex m_mtx;
	std::condition_variable m_cond;

	//
	std::array<T, cap> m_buf{};
	alignas(64) std::uint_fast64_t m_write_pos{};
	alignas(64) std::uint_fast64_t m_read_pos{};
};

#if 0

template<typename T, auto one_chunk_sz = 2048u, auto num_of_chunks = 64u>
class spsc_circ_fifo_ final
{

template <typename Q>
class slot
{
public:
	void write_into(const void* start, std::size_t len)
	{
		if (len > one_chunk_sz) { throw std::runtime_error {"Length is too big"}; }

		std::memcpy(std::get<0>(m_owner->m_buf[m_idx]).data(), start, len);
		std::get<1>(m_owner->m_buf[m_idx]) = len;

		m_owner->m_write_pos = (m_owner->m_write_pos + 1) % num_of_chunks;
		m_owner->m_cond.notify_one();
	}

	std::size_t read_from(void* start, std::size_t len)
	{
		const auto& sz = std::get<1>(m_owner->m_buf[m_idx]);

		if (len < sz) { throw std::runtime_error {"Length is too big"}; }

		std::memcpy(start, std::get<0>(m_owner->m_buf[m_idx]).data(), sz);

		auto l_g = std::lock_guard{m_owner->m_mtx};
		m_owner->m_read_pos = (m_owner->m_read_pos + 1) % num_of_chunks;

		return sz;
	}

private:
	slot(Q* q, std::uint_fast64_t idx)
		: m_owner{q}
		, m_idx{idx}
	{}
	Q* m_owner;
	std::uint_fast64_t m_idx{};
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
#endif

}  // namespace cliph::utils

#endif // spsc_circ_fifo_hpp