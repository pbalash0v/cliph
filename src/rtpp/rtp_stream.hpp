#ifndef rtpp_rtp_stream_hpp
#define rtpp_rtp_stream_hpp

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iosfwd>


namespace rtpp
{
struct payload_type final
{
	payload_type(std::uint16_t clock)
		: m_clock{clock}
	{}
	std::uint16_t m_clock{};
};

class stream final
{
public:
	using pt_type = std::uint8_t;
	using duration_type = std::chrono::milliseconds;
	using payload_map_type = std::unordered_map<pt_type, payload_type>;

public:
	stream();

public:
	void advance_seq_num() noexcept;
	void advance_ts(pt_type, duration_type);
	void* fill(void*, std::size_t, bool mark = false);

	payload_map_type& payloads() & noexcept { return m_payloads; }

public:
	std::ostream& dump(std::ostream&) const;

private:
	std::uint16_t m_seq_num{};
	std::uint32_t m_ssrc{};
	std::uint32_t m_ts{};
	std::uint8_t m_csrc_count{};
	pt_type m_pt{};
	payload_map_type m_payloads;
};


//
inline std::ostream& operator<<(std::ostream& ostr, const stream& s)
{
	return s.dump(ostr);
}
} //namespace rtpp

#endif //rtpp_rtp_stream_hpp
