#ifndef ts_circ_buf_hpp
#define ts_circ_buf_hpp


#include "common_std.hpp"


namespace cliph::utils
{

template<typename T, auto chunk_sz = 2048u, auto capacity = 64u>
class ts_circ_buf final
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
using audio_circ_buf = ts_circ_buf<std::int16_t>;

} // namespace cliph::utils

#endif // ts_circ_buf_hpp