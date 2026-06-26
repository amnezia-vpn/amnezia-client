#include "failover/proxy_list.h"

#include "crypto/aes.h"
#include "crypto/hash.h"
#include "util/base64.h"
#include "util/json.h"

namespace agw::failover
{
    namespace
    {
        void appendStorageUrls(const std::vector<std::string> &baseUrls, const FailoverContext &ctx,
                               std::vector<std::string> &target)
        {
            if (!ctx.serviceType.empty()) {
                const std::string token = "endpoints-" + ctx.serviceType + "-" + ctx.userCountryCode;
                const std::string encoded =
                        util::base64UrlEncodeNoPad(std::vector<std::uint8_t>(token.begin(), token.end()));
                for (const auto &base : baseUrls) {
                    target.push_back(base + encoded + ".json");
                }
            }
            for (const auto &base : baseUrls) {
                target.push_back(base + "endpoints.json");
            }
        }

        std::vector<std::string> parseEndpointsArray(const std::string &json)
        {
            std::vector<std::string> out;
            try {
                util::Json doc = util::Json::parse(json);
                if (doc.is_array()) {
                    for (const auto &el : doc) {
                        if (el.is_string()) {
                            out.push_back(el.get<std::string>());
                        }
                    }
                }
            } catch (...) {
            }
            return out;
        }
    }

    std::vector<std::string> buildStorageUrls(const std::vector<std::string> &primaryBaseUrls,
                                              const std::vector<std::string> &fallbackBaseUrls,
                                              const FailoverContext &ctx)
    {
        std::vector<std::string> result;
        appendStorageUrls(primaryBaseUrls, ctx, result);
        appendStorageUrls(fallbackBaseUrls, ctx, result);
        return result;
    }

    std::vector<std::string> decodeProxyList(const std::string &body, bool isDevEnvironment, const std::string &pubKeyPem)
    {
        if (isDevEnvironment) {
            return parseEndpointsArray(body);
        }

        const std::vector<std::uint8_t> pubBytes(pubKeyPem.begin(), pubKeyPem.end());
        const std::string h = crypto::toHex(crypto::sha512(pubBytes));
        const std::vector<std::uint8_t> key = crypto::fromHex(h.substr(0, 64));
        const std::vector<std::uint8_t> iv = crypto::fromHex(h.substr(64, 32));

        const std::vector<std::uint8_t> cipher = util::base64Decode(body);
        const std::vector<std::uint8_t> plain = crypto::aesDecryptCbc(cipher, key, iv);
        return parseEndpointsArray(std::string(plain.begin(), plain.end()));
    }
}
