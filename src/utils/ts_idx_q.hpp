#ifndef ts_idx_q_hpp
#define ts_idx_q_hpp

#include <cassert>
#include <cstdio>
#include <memory>
#include <mutex>
#include <map>
#include <numeric>
#include <utility>
#include <queue>
#include <vector>
#ifndef NDEBUG
#include <iostream>
#endif
//
#include "common_std.hpp"

namespace cliph::utils
{
#define PQ_INDEXING
//#define DBG_MAP

template<typename T>
class ts_idx_queue final
{
public:
	using wrapped_container_type = T;
	using value_type = typename wrapped_container_type::value_type;
	using queue_type = ts_idx_queue<wrapped_container_type>;

public:	
	class slot
	{
	public:
		value_type& operator*()
		{
			assert(m_idx);
			return *m_idx;
		}

		value_type* operator->()
		{
			assert(m_idx);
			return m_idx;
		}

		slot(const slot& oth) = delete;
		slot& operator=(const slot&) = delete;

		slot(slot&& oth) noexcept
			: m_owner{std::exchange(oth.m_owner, nullptr)}
			, m_idx{std::exchange(oth.m_idx, nullptr)}
		{
		}

		slot& operator=(slot&& oth) noexcept
		{
			if (&oth == this) { return *this; }
			m_owner = std::exchange(oth.m_owner, nullptr);
			m_idx = std::exchange(oth.m_idx, nullptr);
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return m_idx != nullptr && m_owner != nullptr;
		}

		~slot() noexcept
		{
			if (!m_idx) { return; }

			auto lk = std::lock_guard{m_owner->m_mtx};
			m_owner->m_indices.push(m_idx);
		}

		slot() = default;
private:
		slot(queue_type* q, value_type* idx)
			: m_owner{q}
			, m_idx{idx}
		{
		}
		//
		queue_type* m_owner{nullptr};
		value_type* m_idx{nullptr};
		friend class ts_idx_queue<T>;
	};

public:
	using slot_type = typename ts_idx_queue<wrapped_container_type>::slot;

public:
	ts_idx_queue(T& wrapped)
		: m_wrapped{wrapped}
	{
		//for (auto i = 0u; i < m_wrapped.size(); ++i)
		for (auto& elem : m_wrapped)
		{
			m_indices.push(std::addressof(elem));
		}
	}

	[[nodiscard]] slot_type get()
	{
		while (!m_mtx.try_lock())
		{
			std::this_thread::yield();
		}
		auto lk = std::lock_guard{m_mtx, std::adopt_lock};
		//
		if (m_indices.empty())
		{
			return {};
		}
		//
		auto* idx = m_indices.top();
		assert(idx);
		m_indices.pop();

#ifdef DBG_SLOT_USAGE_STAT
		m_stat[idx]++;
#endif
		return {this, idx};
	}

#ifdef DBG_SLOT_USAGE_STAT
	~ts_idx_queue()
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
	std::priority_queue<value_type*, std::vector<value_type*>, std::greater<value_type*>> m_indices;
	//
#ifdef DBG_SLOT_USAGE_STAT
	std::map<value_type*, size_t> m_stat;
#endif
	//
	std::mutex m_mtx;
	std::condition_variable m_cond;
};


template <typename T, auto capacity>
class ts_cbuf
{
public:
	void put(T&& val)
	{
		auto v = std::move(val);
		while (!try_put_impl(v))
		{
		}
	}

	bool try_put(T&& val)
	{
		auto v = std::move(val);
		return try_put_impl(v);
	}

	void get(T& val)
	{
		if (try_get(val))
		{
			return;
		}

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

private:
	bool try_put_impl(T& val)
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

		m_buf[m_write] = std::move(val);
		m_write = (m_write + 1) % capacity;

		lk.unlock();
		m_cond.notify_one();
		return true;
	}
};  //class circ_buf


}  // namespace cliph::utils

#endif // ts_idx_q_hpp