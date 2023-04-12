#ifndef ts_idx_q_hpp
#define ts_idx_q_hpp

#include <cstdio>
#include <mutex>
#include <map>
#include <utility>
#include <queue>
//
#include "common_std.hpp"

namespace cliph::utils
{

template<typename T>
class ts_idx_q final
{
public:
	using container_type = T;
	using queue_type = ts_idx_q<container_type>;
	using value_type = typename container_type::value_type;

public:	
	template <typename Q>
	class slot
	{
	public:
		using value_type = typename Q::value_type;

	public:
		value_type& operator*()
		{
			return m_owner->m_wrapped[m_idx];
		}

		value_type* operator->()
		{
		 	return std::addressof(m_owner->m_wrapped[m_idx]);
		}

		slot(const slot& oth) = delete;
		slot& operator=(const slot&) = delete;

		slot(slot&& oth)
			: m_owner{std::exchange(oth.m_owner, nullptr)}
			, m_idx{std::exchange(oth.m_idx, -1)}
		{
		}

		slot& operator=(slot&& oth) noexcept
		{
			if (&oth == this) { return *this; }
			m_owner = std::exchange(oth.m_owner, nullptr);
			m_idx = std::exchange(oth.m_idx, -1);
			return *this;
		}

		explicit operator bool() const noexcept { return m_idx != -1; }

		~slot()
		{
			if (!*this) { return; }

			auto lk = std::lock_guard{m_owner->m_mtx};
			m_owner->m_index.push(m_idx);
		}

		slot() = default;
		slot(Q* q, int idx)
			: m_owner{q}
			, m_idx{idx}
		{}
		//
		Q* m_owner{};
		int m_idx{-1};
		//
		template<typename> friend class ts_idx_q;
	};

public:
	using slot_type = typename ts_idx_q<container_type>::slot<queue_type>;

public:
	ts_idx_q(T& wrapped)
		: m_wrapped{wrapped}
	{
		for (auto i = 0u; i < m_wrapped.size(); ++i)
		{
			m_index.push(i);
		}
	}

	[[nodiscard]] slot<queue_type> get()
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto lk = std::lock_guard{m_mtx, std::adopt_lock};
		//
		if (m_index.empty())
		{
			return {this, -1};
		}
		auto idx = m_index.top();
		m_index.pop();
		//m_stat[idx]++;
		return {this, static_cast<int>(idx)};
	}

#if 0
	~ts_idx_q()
	{
		for (auto [idx, freq] : m_stat)
		{
			std::cerr << idx << ":" << freq << '\n';
		}
	}
#endif

private:
	T& m_wrapped;

private:
	std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_index;
	//std::map<size_t, size_t> m_stat;
	//
	std::mutex m_mtx;
	std::condition_variable m_cond;
};


template <typename T, auto capacity = 64u>
class ts_cbuf
{
public:
	void put(T&& val)
	{
		while (!(try_put(std::move(val))))
		{
		}
	}

	bool try_put(T&& val)
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto lk = std::unique_lock{m_mtx, std::adopt_lock};

		if (((m_write + 1) % capacity) == m_read)
		{
			return false;
		}
		else
		{
			m_buf[m_write] = std::move(val);
			m_write = (m_write + 1) % capacity;
			lk.unlock();
			m_cond.notify_one();
			return true;
		}
	}

	void get(T& val)
	{
		auto lk = std::unique_lock{m_mtx};
		m_cond.wait(lk, [&]() { return m_write != m_read; });

		val = std::move(m_buf[m_read]);
		m_read = (m_read + 1) % capacity;
	}

	bool try_get(T& val)
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto lk = std::lock_guard{m_mtx, std::adopt_lock};

		if (m_write == m_read) { return false; }

		val = std::move(m_buf[m_read]);
		m_read = (m_read + 1) % capacity;
		return true;
	}

private:
	std::array<T, capacity> m_buf{};

private:
	std::mutex m_mtx;
	std::condition_variable m_cond;
	//	
	alignas(64) std::uint_fast64_t m_read{};
	alignas(64) std::uint_fast64_t m_write{};
};  //class circ_buf


}  // namespace cliph::utils

#endif // ts_idx_q_hpp