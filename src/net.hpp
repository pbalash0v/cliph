#ifndef cliph_net_hpp
#define cliph_net_hpp

#include <cstdint>
//
#include "asio/ip/address.hpp"

//

namespace cliph::net
{
std::vector<asio::ip::address> get_interfaces();
} //namespace cliph::net

#endif // cliph_net_hpp