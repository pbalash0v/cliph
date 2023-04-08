#ifndef ts_mem_chunk_hpp
#define ts_mem_chunk_hpp

#include "common_std.hpp"

namespace cliph::utils
{

template<auto capacity_ = 4096u>
class ts_mem_chunk
{
public:
	void put(const void* src, std::size_t len)
	{
		put_impl(src, len);
	}

	bool put_for(const void* src, std::size_t len
		, std::chrono::nanoseconds until)
	{
		return put_impl(src, len, until);
	}

	[[nodiscard]] std::size_t get(void* src, std::size_t len)
	{
		return *get_impl(src, len);
	}

	[[nodiscard]] std::optional<std::size_t> get_for(void* src, std::size_t len
		, std::chrono::nanoseconds until)
	{
		return get_impl(src, len, until);
	}

	constexpr auto capacity() const noexcept { return capacity_; }

private:
	std::array<std::uint8_t, capacity_> m_buffer{};
	std::size_t m_size{};
	bool m_done{false};
	//
	std::mutex m_mtx;
	std::condition_variable m_cond;

private:
	bool put_impl(const void* src, std::size_t len
		, std::optional<std::chrono::nanoseconds> until = std::nullopt)
	{
		if (len > capacity_) { throw std::runtime_error {"Length is too big"}; }

		auto start = std::chrono::steady_clock::now();

		while (true)
		{
			if (until && std::chrono::steady_clock::now() - start > *until) { return false; }

			if (m_mtx.try_lock())
			{
				if (m_size == 0) { break; } // keep mtx locked
				else { m_mtx.unlock(); }
			}
			else
			{
				std::this_thread::yield();
			}
		}

		//
		{
			auto l = std::lock_guard{m_mtx, std::adopt_lock};
			if (len && src)
			{
				std::memcpy(m_buffer.data(), src, len);
				m_size = len;
			}
			else
			{
				m_done = true;
			}
		}

		m_cond.notify_one();

		return true;
	}

	[[nodiscard]] std::optional<std::size_t> get_impl(void* src, std::size_t len
		, std::optional<std::chrono::nanoseconds> until = std::nullopt)
	{
		if (src == nullptr) return 0;

		auto lk = std::unique_lock{m_mtx};
		auto pred = [&]{ return m_done || m_size > 0; };
		if (until)
		{
			if (!m_cond.wait_for(lk, *until, std::move(pred))) { return std::nullopt; }
		}
		else
		{
			m_cond.wait(lk, std::move(pred));
		}
		
		if (len < m_size) { throw std::runtime_error {"Short write"}; }		

		if (m_done) { return 0; }

		std::memcpy(src, m_buffer.data(), m_size);
		auto written = m_size;
		m_size = 0;

		return written;
	}
};

} //namespace cliph::utils

#endif // ts_mem_chunk_hpp