#ifndef audio_engine_hpp
#define audio_engine_hpp

//
#include "asio/ip/address.hpp"
#include <string_view>
#include <cstdint>
//
#include <miniaudio.h>
//

namespace cliph
{

struct config
{
	asio::ip::address net_iface;
};

class engine final
{
public:
	static engine& get() noexcept;

public:
	~engine();

public:
	engine& init(const config&);
	//
	void start();
	void pause();
	void stop();
	//
	void set_net_sink(std::string_view, std::uint16_t);
	void set_remote_opus_params(std::uint8_t);
	//
	std::string description() const;

private:
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	engine() = default;
	void enumerate_and_select();
};

} // namespace cliph::audio

#endif // audio_engine_hpp