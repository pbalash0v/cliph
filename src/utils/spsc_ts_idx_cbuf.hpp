#ifndef spsc_ts_idx_cbuf_hpp
#define spsc_ts_idx_cbuf_hpp

#include "common_std.hpp"

namespace cliph::utils
{

template<auto capacity = 64u>
class spsc_ts_idx_cbuf final
{
public:
	using index_type = std::uint_fast64_t;
	constexpr decltype(capacity) cap() const noexcept
	{
		return capacity;
	}

private:	
	//
	std::mutex m_mtx;
	std::condition_variable m_cond;
	//
	alignas(64) index_type m_write_pos{};
	alignas(64) index_type m_read_pos{};	

public:
	[[nodiscard]] std::optional<index_type> get_write_idx()
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}

		auto l = std::lock_guard{m_mtx, std::adopt_lock};
		if (((m_write_pos + 1) % capacity) == m_read_pos)
		{
			return std::nullopt;
		}

		return m_write_pos;
	}

	void commit_write(index_type idx)
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		{
			auto l = std::unique_lock{m_mtx, std::adopt_lock};
			m_write_pos = (m_write_pos + 1) % capacity;
		}
		m_cond.notify_one();
	}

	[[nodiscard]] index_type get_read_idx()
	{
		auto l = std::unique_lock{m_mtx};
		m_cond.wait(l, [&] { return m_write_pos != m_read_pos; });
		return m_read_pos;
	}

	void commit_read(index_type idx)
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		{
			auto l = std::unique_lock{m_mtx, std::adopt_lock};
			m_read_pos = (m_read_pos + 1) % capacity;
		}
		m_cond.notify_one();
	}
};



} //namespace cliph::utils

#endif // spsc_ts_idx_cbuf_hpp