#include "controller.hpp"

namespace
{

} //namespace


namespace cliph
{
controller& controller::get() noexcept
{
	static auto instance = controller{};
	return instance;
}

std::string controller::description() const
{
	return cliph::sdp::get_local(local_media.to_string(), sock_ptr->port()).c_str();
}

} // namespace cliph
