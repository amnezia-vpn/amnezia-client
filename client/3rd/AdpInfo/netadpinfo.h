#pragma once 

#include <vector>
#include <string>
#include <tuple>
#include <memory>


namespace  adpinfo{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//static bool is_string_equal(const std::string &lhs, const std::string &rhs){
//    if (lhs.find(rhs) != std::string::npos)
//        return true;
//    return false;
//}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// {false,""} - no error
// {true,"descr"} - error with description
using RET_TYPE = std::tuple<bool, std::string>;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//class Adapter{
//private:
//    std::string name{};
//    std::string descr{};
//    std::string current_ip_address_v4{};
//    std::string maskv4{};
//    std::vector<std::string> dns_address{};

//public:
//    explicit Adapter() = default;
//    ~Adapter() = default;

//    void set_name(std::string_view);
//    std::string_view get_name()const;

//    void set_description(std::string_view);
//    std::string_view get_description()const;

//    void set_mac(std::string_view);
//    std::string_view get_mac()const;

////    bool operator==(const adapter& rhs) {
////        if (!is_string_equal(name, rhs.name))
////            return false;
////        if (!is_string_equal(mac, rhs.mac))
////            return false;
////        if (dns_address != rhs.dns_address)
////            return false;
////        return true;
////    }
//}adapter;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
 * The object uses for collect the information about active network adapters/interfaces
 *  QString m_routeGateway;
    QString m_vpnLocalAddress;
    QString m_vpnGateway;
*/
class NetAdpInfo final{
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class Adapter{
        std::string name{};
        std::string descr{};
        std::string route{};
        std::string address{};
        std::string gateway{};
    public:
        explicit Adapter() = default;
        ~Adapter() = default;

        void set_name(std::string_view);
        std::string_view get_name()const;
        void set_description(std::string_view);
        std::string_view get_description()const;
        void set_route_gateway(std::string_view);
        std::string_view get_route_gateway()const;
        void set_local_address(std::string_view);
        std::string_view get_local_address()const;
        void set_local_gateway(std::string_view);
        std::string_view get_local_gateway()const;
    };
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    int16_t _index_of_adapter{};
    std::vector<std::shared_ptr<Adapter>>_adapters{};

    RET_TYPE collect_adapters_data();

public:
    explicit NetAdpInfo() = default;
    ~NetAdpInfo()  = default;

    RET_TYPE get_adapter_infor(std::string_view );
    std::string_view get_adapter_route_gateway()const;
    std::string_view get_adapter_local_address()const;
    std::string_view get_adapter_local_gateway()const;
    
    //static std::string get_system_route();
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
} //end namespace
