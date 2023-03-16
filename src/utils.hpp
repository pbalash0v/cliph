#ifndef circular_hpp
#define circular_hpp

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace cliph::utils
{

struct thread_safe_array
{
	inline static constexpr const auto k_capacity{2048u};
	std::size_t m_size{};
	std::array<std::uint8_t, k_capacity> m_buffer;
	bool m_is_empty{true};
	std::mutex m_mtx;
	std::condition_variable m_cond;	
};

} //namespace cliph::utils

#endif // circular_hpp