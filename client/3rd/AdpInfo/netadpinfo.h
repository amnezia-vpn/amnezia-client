#pragma once 

#include <vector>
#include <string>
#include <tuple>
#include <memory>


namespace  adpinfo{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// {false,""} - no error
// {true,"descr"} - error with description
using RET_TYPE = std::tuple<bool, std::string>;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
 * The object uses for collect the information about active network adapters/interfaces
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

    RET_TYPE get_adapter_info(std::string_view );
    std::string_view get_adapter_route_gateway()const;
    std::string_view get_adapter_local_address()const;
    std::string_view get_adapter_local_gateway()const;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
} //end namespace
