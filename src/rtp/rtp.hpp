#ifndef rtp_hpp
#define rtp_hpp

#include <iosfwd>
#include <cstdint>

namespace cliph::rtp
{

class rtp final
{
public:
	rtp(void*, std::size_t);

public:
	// read rtp packet params from provided memory location
	[[nodiscard]] bool is_padded() const noexcept;
	[[nodiscard]] bool has_extensions() const noexcept;
	[[nodiscard]] bool has_mark() const noexcept;
	[[nodiscard]] std::uint8_t pt() const noexcept;
	[[nodiscard]] std::uint16_t seq_num() const noexcept;
	[[nodiscard]] std::uint32_t ts() const noexcept;
	[[nodiscard]] std::uint32_t ssrc() const noexcept;
	[[nodiscard]] std::uint8_t csrc_count() const noexcept;
	[[nodiscard]] const void* data() const noexcept;
	[[nodiscard]] std::uint16_t size() const noexcept;
	operator const std::uint8_t*() const noexcept;

	// write rtp packet params to provided memory location
	void ver(std::uint8_t = 2u) noexcept;
	void pad(bool) noexcept;
	void extensions(bool) noexcept;
	void mark(bool) noexcept;
	void pt(std::uint8_t);
	void seq_num(std::uint16_t) noexcept;
	void ts(std::uint32_t) noexcept;
	void ssrc(std::uint32_t) noexcept;
	void csrc_count(std::uint8_t);
	void* data() noexcept;

public:
	explicit operator bool() const noexcept;
	std::ostream& dump(std::ostream&) const;

private:
	void* m_start{};
	std::size_t m_len{};
};

//
inline std::ostream& operator<<(std::ostream& ostr, const rtp& rtp)
{
	return rtp.dump(ostr);
}

} //namespace mspg

#endif //rtp_hpp