#ifndef audio_engine_hpp
#define audio_engine_hpp

//
#include <string_view>
#include <cstdint>
//
#include <miniaudio.h>
//

namespace cliph::audio
{

class engine final
{
public:
	static engine& get() noexcept;

public:
	~engine();

public:
	engine& init();
	void start();
	void stop();
	void sink(std::string_view, std::uint16_t);
	std::string description() const;

private:
	unsigned m_dev_id{};
    ma_device m_device;
    ma_context m_context;
    ma_encoder m_encoder;

private:
	engine() = default;
	void enumerate_and_select();
};

} // namespace cliph::audio

#endif // audio_engine_hpp