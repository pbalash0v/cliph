#ifndef cliph_sdp_hpp
#define cliph_sdp_hpp

#include "rutil/DataStream.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/TrickleIceContents.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "rutil/ParseBuffer.hxx"

#include <iostream>

namespace cliph::sdp
{
using namespace resip;
using namespace std;

constexpr const auto* const k_ip_addr = "127.0.0.1";

inline resip::Data get(std::string ip_addr = k_ip_addr, std::uint16_t port = 9)
{
	auto origin = SdpContents::Session::Origin{"-", 0, 1, SdpContents::IP4, "0.0.0.0"};
	auto session = SdpContents::Session{0, origin, "cliph"};
	session.connection() = SdpContents::Session::Connection(SdpContents::IP4, ip_addr.c_str());
	//
	auto audio_section = SdpContents::Session::Medium{};
	audio_section.name() = "audio";
	audio_section.port() = port;
	audio_section.protocol() = "RTP/AVP";
      //
      audio_section.addFormat("96");
      audio_section.addAttribute("rtpmap", "96 OPUS/48000/2");
      //
#if 0
	audio_section.addCodec(SdpContents::Session::Codec::getStaticCodecs().at(8));
#endif
      //
#if 0
      audio_section.addFormat("101");
      audio_section.addAttribute("rtpmap", "101 telephone-event/8000");
      audio_section.addAttribute("fmtp", "101 0-11");
#endif      
      audio_section.addAttribute(SdpContents::Session::Direction::SENDRECV.name());
	session.addMedium(audio_section);
	//
	auto sdp = SdpContents{};
	sdp.session() = session;

	return Data::from(sdp);
}

} // namespace

#endif // cliph_sdp_hpp
