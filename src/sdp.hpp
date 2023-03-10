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

const auto* const k_ip_addr = "127.0.0.1";

inline resip::Data get(std::string ip_addr = k_ip_addr)
{

	auto origin = SdpContents::Session::Origin{"-", 0, 1, SdpContents::IP4, "0.0.0.0"};
	auto session = SdpContents::Session{0, origin, "cliph"};
	session.connection() = SdpContents::Session::Connection(SdpContents::IP4, ip_addr.c_str());
	//
	auto audio_section = SdpContents::Session::Medium{};
	audio_section.name() = "audio";
	audio_section.port() = 9u;
	audio_section.protocol() = "RTP/AVP";
	audio_section.addAttribute(SdpContents::Session::Direction::SENDONLY.name());
	//
	auto opus = SdpContents::Session::Codec{"OPUS", 96, 48000};
	//
	audio_section.addCodec(opus);
	//audio_section.addCodec(SdpContents::Session::Codec::getStaticCodecs().at(8));
	session.addMedium(audio_section);
	//
	auto sdp = SdpContents{};
	sdp.session() = session;

	return Data::from(sdp);
}

#if 0
inline void test()
{
      SdpContents sdp;
      unsigned long tm = 4058038202u;
      Data address("192.168.2.220");
      int port = 5061;
   
      unsigned long sessionId((unsigned long) tm);
   
      SdpContents::Session::Origin origin("-", sessionId, sessionId, SdpContents::IP4, address);
   
      SdpContents::Session session(0, origin, "VOVIDA Session");
      
      session.connection() = SdpContents::Session::Connection(SdpContents::IP4, address);
      session.addTime(SdpContents::Session::Time(tm, 0));
      
      SdpContents::Session::Medium medium("audio", port, 0, "RTP/AVP");
      medium.addFormat("0");
      medium.addFormat("101");
      
      medium.addAttribute("rtpmap", "0 PCMU/8000");
      medium.addAttribute("rtpmap", "101 telephone-event/8000");
      medium.addAttribute("ptime", "160");
      medium.addAttribute("fmtp", "101 0-11");
      
      session.addMedium(medium);
      
      sdp.session() = session;

      Data shouldBeLike("v=0\r\n"
                        "o=- 4058038202 4058038202 IN IP4 192.168.2.220\r\n"
                        "s=VOVIDA Session\r\n"
                        "c=IN IP4 192.168.2.220\r\n"
                        "t=4058038202 0\r\n"
                        "m=audio 5061 RTP/AVP 0 101\r\n"
                        "a=fmtp:101 0-11\r\n"
                        "a=ptime:160\r\n"
                        "a=rtpmap:0 PCMU/8000\r\n"
                        "a=rtpmap:101 telephone-event/8000\r\n");

      Data encoded(Data::from(sdp));

      cout << encoded;
      cout << shouldBeLike;
}
#endif

} // namespace

#endif // cliph_sdp_hpp
