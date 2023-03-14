#ifndef rtcp_hpp
#define rtcp_hpp

#include <iostream>
#include <cstdint>


namespace mspg
{
class rtcp final
{
public:
	rtcp(void*, std::size_t);

public:
	[[nodiscard]] std::uint8_t ver() const noexcept;
	[[nodiscard]] bool is_padded() const noexcept;

public:
	operator bool() const noexcept;

private:
	void* m_start{};
	std::size_t m_len{};

private:
	friend std::ostream& operator<<(std::ostream& ostr, const rtcp& rtcp)
	{
		if (!rtcp) { ostr << "[Invalid RTCP]"; return ostr; }

		ostr << std::boolalpha
			<< "RTCP [v:" << +rtcp.ver()
			<< ", p:" << rtcp.is_padded()
#if 0			
			<< ", mark: " << rtp.has_mark()
			<< ", extensions: " << rtp.has_extensions()
			<< ", pt: " << +rtp.pt()
			<< ", seq_num: " << +rtp.seq_num()
			<< ", time stamp: " << +rtp.t_stamp()
			<< ", ssrc: " << +rtp.ssrc()
			<< ", start: " << +(*rtp.m_start)
			<< ", start: " << *rtp.m_start
			<< ", data: " << +rtp.data()
#endif
			<< "]";
		
		return ostr;
	}

};
} //namespace mspg

#endif //rtcp_hpp