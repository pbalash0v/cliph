#ifndef cliph_rtp_hpp
#define cliph_rtp_hpp

#include <cstdint>
#include <thread>

//
#include "data_types.hpp"
#include "rtp_stream.hpp"

namespace cliph::rtp
{

class engine final
{
public:
	engine(data::media_buf&, data::media_buf&);
	~engine();
	void run();

private:
	data::media_buf& m_in_buf;
	data::media_buf& m_out_buf;

private:
	std::thread m_egress_thr;
	std::thread m_ingress_thr;
	//!
	rtpp::stream m_strm;

private:
	void egress_loop();
};

} // namespace cliph::rtp

#endif