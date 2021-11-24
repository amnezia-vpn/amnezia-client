#include "netadpinfo.h"

#include <QDebug>

#include <algorithm>
#include <iterator>
#include <cassert>

#include <windows.h>
//#include <atlbase.h>
//#include <wbemcli.h>
//#include <comutil.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "wbemuuid.lib")
//#pragma comment(lib, "comsuppw.lib")

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static std::string convert_wide_to_ansi(const std::wstring& widestring) {
    auto nchars = WideCharToMultiByte(
                CP_ACP,
                0,
                widestring.c_str(),
                static_cast<int>(widestring.length() + 1),
                nullptr,
                0,
                nullptr,
                nullptr);
    std::string converted_string{};
    converted_string.resize(nchars);
    WideCharToMultiByte(CP_ACP,
                        0,
                        widestring.c_str(),
                        -1,
                        &converted_string[0],
            static_cast<int>(widestring.length()),
            nullptr,
            nullptr);
    return converted_string;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static std::string get_route_gateway()
{
    std::string route_gateway{};
    PMIB_IPFORWARDTABLE pIpForwardTable{nullptr};
    DWORD dwSize = 0;
    BOOL bOrder = FALSE;

    struct in_addr IpAddr;
    char szGatewayIp[128]{'\0'};

    DWORD dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
        // Allocate the memory for the table
        if (!(pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc(dwSize))) {
            //printf("Malloc failed. Out of memory.\n");
            return {"Out of memory"};
        }
        // Now get the table.
        dwStatus = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
    }
    if (dwStatus != ERROR_SUCCESS) {
        if (pIpForwardTable)
            free(pIpForwardTable);
        return {"getIpForwardTable failed"};
    }
    const DWORD end = pIpForwardTable->dwNumEntries;
    for (DWORD i = 0; i < end; i++) {
        if (pIpForwardTable->table[i].dwForwardDest == 0) {
            // We have found the default gateway.
            IpAddr.S_un.S_addr =
                    (u_long) pIpForwardTable->table[i].dwForwardNextHop;
            strcpy_s(szGatewayIp, sizeof (szGatewayIp), inet_ntoa(IpAddr));
            route_gateway = std::string(szGatewayIp);
            break;
        }
    }
    if (pIpForwardTable)
        free(pIpForwardTable);

    return route_gateway;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
namespace adpinfo {

void NetAdpInfo::Adapter::set_name(std::string_view value){
    name = value;
}
std::string_view NetAdpInfo::Adapter::get_name()const{

    return name;
}
void NetAdpInfo::Adapter::set_description(std::string_view value){
    descr = value;
}
std::string_view NetAdpInfo::Adapter::get_description()const{
    return descr;
}
void  NetAdpInfo::Adapter::set_route_gateway(std::string_view value){
    route = value;
}
std::string_view NetAdpInfo::Adapter::get_route_gateway()const{
    return route;

}
void NetAdpInfo::Adapter::set_local_address(std::string_view value){
    address = value;
}
std::string_view NetAdpInfo::Adapter::get_local_address()const{
    return address;
}
void NetAdpInfo::Adapter::set_local_gateway(std::string_view value){
    gateway = value;
}
std::string_view NetAdpInfo::Adapter::get_local_gateway()const{
    return gateway;
}

RET_TYPE NetAdpInfo::collect_adapters_data(){
    _adapters.clear();
    std::vector<BYTE> buffer{};
    IP_ADAPTER_INFO *adapter_info{nullptr};
    DWORD result{ERROR_BUFFER_OVERFLOW};
    ULONG buffer_len = sizeof(IP_ADAPTER_INFO) * 10;
    while (result == ERROR_BUFFER_OVERFLOW){
        buffer.resize(buffer_len);
        adapter_info = reinterpret_cast<IP_ADAPTER_INFO*>(&buffer[0]);
        result = GetAdaptersInfo(adapter_info, &buffer_len);
        if (result == ERROR_NO_DATA){
            return {true, "GetAdaptersInfo return ERROR_NO_DATA"};
        }
    }//end while
    if (result != NO_ERROR){
        const std::string &error = "GetAdaptersInfo failed :" + std::to_string(result);
        return {true, error};
    }
    IP_ADAPTER_INFO *adapter_iterator = adapter_info;
    while(adapter_iterator){
        std::shared_ptr<Adapter>_tmp{std::make_shared<Adapter>()};
        _tmp->set_name(adapter_iterator->AdapterName);
        _tmp->set_description(adapter_iterator->Description);
        _tmp->set_local_address(adapter_iterator->IpAddressList.IpAddress.String);
        std::string lgw = adapter_iterator->GatewayList.IpAddress.String;
        if (lgw.length() == 0 || lgw.find("0.0.0.0") != std::string::npos)
        {
            if (adapter_iterator->DhcpEnabled == 1)
            {
                lgw = adapter_iterator->DhcpServer.IpAddress.String;
            }
        }
        _tmp->set_local_gateway(lgw);
        //_tmp->set_local_gateway(adapter_iterator->GatewayList.IpAddress.String);
        _tmp->set_route_gateway(get_route_gateway());
        _adapters.emplace_back(_tmp);
        adapter_iterator = adapter_iterator->Next;
    }
    return {false, ""};
}

RET_TYPE NetAdpInfo::get_adapter_info(std::string_view _adapter_name){

    _index_of_adapter = -1;
    const auto result{collect_adapters_data()};
    if (std::get<0>(result) == true){
        _index_of_adapter = -1;
        return result;
    }
    const int16_t &len = static_cast<int16_t>(_adapters.size());
    for (auto i = 0; i< len; ++i){
        auto adap_name = _adapters[i]->get_name();
        auto adap_desc = _adapters[i]->get_description();
        qDebug()<<"adap name : "<<QString::fromStdString(adap_name.data());
        qDebug()<<"adap description : "<<QString::fromStdString(adap_desc.data());
        qDebug()<<"find_string: "<<QString::fromStdString(_adapter_name.data());
        if (adap_name.find(_adapter_name) != std::string::npos || adap_desc.find(_adapter_name) != std::string::npos){
            _index_of_adapter = i;
            return {false, ""};
        }
    }
    return {true, "adapters no founded"};
}

std::string_view NetAdpInfo::get_adapter_route_gateway()const{
    if (_index_of_adapter < 0)
        return "error adapter index";
    return _adapters.at(_index_of_adapter)->get_route_gateway();
}
std::string_view NetAdpInfo::get_adapter_local_address()const{
    if (_index_of_adapter < 0)
        return "error adapter index";
    return _adapters.at(_index_of_adapter)->get_local_address();
}
std::string_view NetAdpInfo::get_adapter_local_gateway()const{
    if (_index_of_adapter < 0)
        return "error adapter index";
    return _adapters.at(_index_of_adapter)->get_local_gateway();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//std::string NetAdpInfo::get_system_route(){
//    return  get_route_gateway();
//}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}// end namespace
