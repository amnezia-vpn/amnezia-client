#pragma once 

#include <list>
#include <vector>
#include <string>
#include <tuple>
//#include <thread>

namespace  adpinfo{

static bool is_string_equal(const std::string &lhs, const std::string &rhs){
    if (lhs.find(rhs) != std::string::npos)
        return true;
    return false;
}
using RET_TYPE = std::tuple<bool, std::string>;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct adapter{
    std::string name{};
    std::string mac{};
    std::list<std::string> dns_address{};
    std::list<std::tuple<std::string,std::string>> ip_mask_V4{};
    std::list<std::string> gateWay{};
    std::string dhcp_server_adress{};

    std::string system_gateway{}; // default system gatewasy for ip 0.0.0.0

    bool operator==(const adapter& rhs) {
        if (!is_string_equal(name, rhs.name))
            return false;
        if (!is_string_equal(mac, rhs.mac))
            return false;
        if (dns_address != rhs.dns_address)
            return false;
        if (ip_mask_V4 != rhs.ip_mask_V4)
            return false;
        if (gateWay != rhs.gateWay)
            return false;
        if (!is_string_equal(dhcp_server_adress, rhs.dhcp_server_adress))
            return false;
        if (!is_string_equal(system_gateway, rhs.system_gateway))
            return false;

        return true;
    }
}adapter;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
 * The object uses for collect the information about active network adapters/interfaces
 * NetAdpInfo adi;
 * //Each call to the collect function fills
 * //the _adapters array with information about active network connections (network address, gateways, ip addresses, etc.).
 * adi.collect();
 *
 * //We get the adapter that appeared after the penultimate call to the collect function
 * adi.get_differents()
 *
 * //We get the adapter that appeared after the penultimate call to the collect function and
 * //have a concrete system name
 *get_adapter_info_by_name
*/
class NetAdpInfo final{

    bool init_error{false};
    std::vector<adapter>_adapters{};
    adapter _adapter_in_use{};

public:
    explicit NetAdpInfo();
    ~NetAdpInfo();
    
    //collect all adapter data
    RET_TYPE collect();
    //return all adapter data
    std::list<adapter> get_adapters()const;
    //get the diferent between the first call collect and the next call collect
    adapter get_differents()const;
    //get the adapter by name
    adapter get_adapter_info_by_name(const std::string &)const;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}
