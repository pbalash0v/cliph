#ifndef ts_array_hpp
#define ts_array_hpp

#include "common_std.hpp"


namespace cliph::utils
{

//TODO: implement proper incapsulation in class
struct ts_array
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

inline void ts_array::mark_empty_locked()
{
	m_is_empty = true;
	m_size = 0;
}
inline void ts_array::mark_filled_locked(std::size_t sz)
{
	m_is_empty = false;
	m_size = sz;
}
//
inline void ts_array::lock()
{
	m_mtx.lock();
}
inline void ts_array::unlock()
{
	m_mtx.unlock();
}
inline auto ts_array::try_lock() noexcept
{
	return m_mtx.try_lock();
}


} //namespace cliph::utils

#endif // ts_array_hpp