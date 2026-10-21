#ifndef VPNURLHANDLER_H
#define VPNURLHANDLER_H

class VpnUrlHandler
{
public:
    enum class Status {
        IsDefault,
        NotRegistered,
        OtherHandler
    };

    static Status status();

    static bool setAsDefault();

    static void unregister();
};

#endif // VPNURLHANDLER_H
