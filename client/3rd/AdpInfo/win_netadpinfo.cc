#include "netadpinfo.h"

#include <QDebug>

#include <algorithm>
#include <iterator>
#include <cassert>

#include <windows.h>
#include <atlbase.h>
#include <wbemcli.h>
#include <comutil.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")

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
    const auto end = pIpForwardTable->dwNumEntries;
    for (auto i = 0; i < end; i++) {
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
namespace amnezia_wmi {

// prepare WMI object
static adpinfo::RET_TYPE prepare_WMI(CComPtr<IWbemLocator>&pLoc,
                                     CComPtr<IWbemServices> &pSvc,
                                     CComPtr<IEnumWbemClassObject>&pEnumerator){
    HRESULT result;
    {
        result = CoCreateInstance(
                    CLSID_WbemLocator,
                    0,
                    CLSCTX_INPROC_SERVER,
                    IID_IWbemLocator, (LPVOID *)&pLoc);
        if (FAILED(result))
        {
            // TODO: add GetLastError description
            return std::make_tuple(true, "CoCreateInstance failed");
        }
    }

    {
        result = pLoc->ConnectServer(
                    _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
                    NULL,                    // User name. NULL = current user
                    NULL,                    // User password. NULL = current
                    0,                       // Locale. NULL indicates current
                    NULL,                    // Security flags.
                    0,                       // Authority (for example, Kerberos)
                    0,                       // Context object
                    &pSvc                    // pointer to IWbemServices proxy
                    );
        if (FAILED(result))
        {
            // TODO: add GetLastError description
            return std::make_tuple(true, "ConnectServer failed");
        }
    }
    {
        result = CoSetProxyBlanket(
                    pSvc,                        // Indicates the proxy to set
                    RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                    RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                    NULL,                        // Server principal name
                    RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
                    RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                    NULL,                        // client identity
                    EOAC_NONE                    // proxy capabilities
                    );

        if (FAILED(result))
        {
            // TODO: add GetLastError description
            return std::make_tuple(true, "CoSetProxyBlanket failed");
        }
    }
    {
        result = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("Select * from Win32_NetworkAdapterConfiguration WHERE IPEnabled = true"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);
        if (FAILED(result))
        {
            // TODO: add GetLastError description
            return std::make_tuple(true, "ExecQuery failed");
        }
    }
    return std::make_tuple(false,"");
}

// get one WMI parametr from object
static std::string getValue(IWbemClassObject* pclsObj, const std::wstring& ParamName) {
    VARIANT vtProp;
    auto result = pclsObj->Get(ParamName.c_str(), 0, &vtProp, 0, 0);
    if (FAILED(result)) {
        VariantClear(&vtProp);
        return {};
    }
    if (vtProp.vt == VT_EMPTY || vtProp.vt == VT_NULL) {
        VariantClear(&vtProp);
        return {};
    }
    const std::string &retValue =  convert_wide_to_ansi(vtProp.bstrVal) ;
    VariantClear(&vtProp);
    return retValue;
}

// get the array of WMI parametrs from object
static std::vector<std::string>getValues(IWbemClassObject* pclsObj, const std::wstring& ParamName) {
    VARIANT vtProp;
    std::vector<std::string>_values{};
    auto result = pclsObj->Get(ParamName.c_str(), 0, &vtProp, 0, 0);
    if (FAILED(result)) {
        VariantClear(&vtProp);
        return std::vector<std::string>();
    }
    if (vtProp.vt == (VT_ARRAY | VT_BSTR)) {
        SAFEARRAY* pSafeArray{ V_ARRAY(&vtProp) };
        BSTR elm;
        long size = pSafeArray->rgsabound[0].cElements;
        long i = 0;
        for (i = 0; i < size; ++i) {
            SafeArrayGetElement(pSafeArray, &i, (void*)&elm);
            const std::wstring& widestring = elm;
            const std::string& string_str = convert_wide_to_ansi(widestring);
            _values.emplace_back(string_str);
        }
        VariantClear(&vtProp);
        return _values;
    }
    else {
        VariantClear(&vtProp);
        return std::vector<std::string>();
    }
}
}

namespace  adpinfo{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NetAdpInfo::NetAdpInfo()
{
    _adapters.clear();
    SetLastError(0);
    HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(result))
    {
        init_error = true;
        return;
    }

    CoInitializeSecurity(
                NULL,
                -1,                          // COM authentication
                NULL,                        // Authentication services
                NULL,                        // Reserved
                RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
                RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
                NULL,                        // Authentication info
                EOAC_NONE,                   // Additional capabilities
                NULL                         // Reserved
                );
    //    if (result != S_OK || result != RPC_E_TOO_LATE)
    //    {
    //        // TODO: add GetLastError description
    //        //_current_error = std::make_tuple(true, "CoInitializeSecurity failed");
    //        //printf("%d\r\n", __LINE__);
    //        qDebug()<<GetLastError()<<"result= "<<result;
    //        init_error = true;
    //        return;
    //    }
    collect();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NetAdpInfo::~NetAdpInfo()
{
    CoUninitialize();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
RET_TYPE NetAdpInfo::collect()
{
    if (init_error){
        return std::make_tuple(true,"Initial error");
    }
    std::vector<adapter>_adapters_next;
    _adapters_next.clear();
    if (_adapters.size()>0){
        std::copy(_adapters.begin(), _adapters.end(), std::back_inserter(_adapters_next));
        _adapters.clear();
    }

    CComPtr<IWbemLocator> pLoc{nullptr};
    CComPtr<IWbemServices> pSvc{nullptr};
    CComPtr<IEnumWbemClassObject> pEnumerator{nullptr};
    auto wmi_init {amnezia_wmi::prepare_WMI(pLoc, pSvc, pEnumerator)};
    if (std::get<0>(wmi_init)){
        return wmi_init;
    }
    CComPtr<IWbemClassObject> pclsObj{nullptr};
    ULONG uReturn = 0;
    {
        // get data from query
        while (pEnumerator)
        {
            HRESULT result;
            result = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (FAILED(result)){
                break;
            }
            if (uReturn == 0) {
                break;
            }

            auto adap_name = amnezia_wmi::getValue(pclsObj,         L"Description");
            auto adap_mac = amnezia_wmi::getValue(pclsObj,          L"MACAddress");
            auto adap_dns = amnezia_wmi::getValues(pclsObj,         L"DNSServerSearchOrder");
            auto adap_ips_address = amnezia_wmi::getValues(pclsObj, L"IPAddress");
            auto adap_ips_mask = amnezia_wmi::getValues(pclsObj,    L"IPSubnet");
            auto adap_gateway = amnezia_wmi::getValues(pclsObj,     L"DefaultIPGateway");
            auto adap_gate = amnezia_wmi::getValue(pclsObj,         L"DHCPServer");
            {
                adapter ainfo;
                ainfo.name = adap_name;
                ainfo.mac = adap_mac;
                ainfo.dns_address = std::list<std::string>(adap_dns.begin(), adap_dns.end());
                ainfo.gateWay = std::list<std::string>(adap_gateway.begin(), adap_gateway.end());
                ainfo.dhcp_server_adress = adap_gate;
                if (ainfo.gateWay.size() == 0)
                    ainfo.gateWay.push_back(get_route_gateway());

                const auto ipv4_address_size = adap_ips_address.size();
                const auto ipv4_mask_size = adap_ips_mask.size();
                assert(ipv4_address_size == ipv4_mask_size);
                for (auto i = 0; i < ipv4_address_size; ++i) {
                    ainfo.ip_mask_V4.emplace_back(std::make_tuple(adap_ips_address.at(i), adap_ips_mask.at(i)));
                }
                _adapters.emplace_back(ainfo);
            }
            pclsObj.Release();
            pclsObj = nullptr;
        }
    }
    if (_adapters_next.size()>0){
        const auto newsize = _adapters.size();
        const auto oldsize = _adapters_next.size();
        if (newsize>oldsize){
            auto fv = _adapters;
            for (int i = 0; i< newsize ;++i){
                if (i < oldsize){
                    if (_adapters[i] == _adapters_next[i]){
                        fv.erase(fv.begin()+i);
                        continue;
                    }
                }
            }
            _adapter_in_use = *fv.begin();
        }
    }
    return std::make_tuple(false, "");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
std::list<adapter> NetAdpInfo::get_adapters()const {
    return std::list<adapter>(_adapters.begin(), _adapters.end());
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
adapter NetAdpInfo::get_differents()const{
    return _adapter_in_use;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
adapter NetAdpInfo::get_adapter_info_by_name(const std::string &_adap_name)const{

    adapter ainfo;
    if (init_error){
        return ainfo;
    }
    CComPtr<IWbemLocator> pLoc{nullptr};
    CComPtr<IWbemServices> pSvc{nullptr};
    CComPtr<IEnumWbemClassObject> pEnumerator{nullptr};
    auto wmi_init = amnezia_wmi::prepare_WMI(pLoc, pSvc, pEnumerator);
    if (std::get<0>(wmi_init)){
        return ainfo;
    }
    CComPtr<IWbemClassObject> pclsObj{nullptr};
    ULONG uReturn = 0;
    {
        // get data from query
        while (pEnumerator)
        {
            HRESULT result;
            result = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (FAILED(result)){
                break;
            }
            if (uReturn == 0) {
                break;
            }

            auto adap_name = amnezia_wmi::getValue(pclsObj,         L"Description"); // can take Caption
            auto adap_mac = amnezia_wmi::getValue(pclsObj,          L"MACAddress");
            auto adap_dns = amnezia_wmi::getValues(pclsObj,         L"DNSServerSearchOrder");
            auto adap_ips_address = amnezia_wmi::getValues(pclsObj, L"IPAddress");
            auto adap_ips_mask = amnezia_wmi::getValues(pclsObj,    L"IPSubnet");
            auto adap_gateway = amnezia_wmi::getValues(pclsObj,     L"DefaultIPGateway");
            auto adap_gate = amnezia_wmi::getValue(pclsObj,         L"DHCPServer");

            {
                adapter ainfoc;
                ainfoc.system_gateway = get_route_gateway();
                ainfoc.name = adap_name;
                ainfoc.mac = adap_mac;
                ainfoc.dns_address = std::list<std::string>(adap_dns.begin(), adap_dns.end());
                ainfoc.gateWay = std::list<std::string>(adap_gateway.begin(), adap_gateway.end());
                ainfoc.dhcp_server_adress = adap_gate;
                if (ainfoc.gateWay.size() == 0)
                    ainfoc.gateWay.push_back(get_route_gateway());

                const auto ipv4_address_size = adap_ips_address.size();
                const auto ipv4_mask_size = adap_ips_mask.size();
                assert(ipv4_address_size == ipv4_mask_size);
                for (auto i = 0; i < ipv4_address_size; ++i) {
                    ainfoc.ip_mask_V4.emplace_back(std::make_tuple(adap_ips_address.at(i), adap_ips_mask.at(i)));
                }
                qDebug()<<"Current adapter name is ["<<ainfoc.name.data()<<"] but we search for ["<<_adap_name.data()<<"]";
                if ( ainfoc.name.find(_adap_name) != std::string::npos ){
                    qDebug()<<"Find adapter with name = "<<_adap_name.data();
                    pclsObj.Release();
                    pclsObj = nullptr;
                    return ainfoc;
                }
            }
            pclsObj.Release();
            pclsObj = nullptr;
        }
    }
    return ainfo;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}
