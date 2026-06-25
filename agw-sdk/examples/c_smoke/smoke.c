/*
 * Чистый C-потребитель C-ABI: доказывает, что agw_* линкуется и работает из C без C++/Qt.
 * Детерминированный путь без сети: невалидный публичный ключ → ApiMissingAgwPublicKey (1105).
 *
 * Сборка (пример, macOS):
 *   cc -std=c11 -I ../../include smoke.c -L ../../build-local -lagw_capi -o smoke
 *   DYLD_LIBRARY_PATH=../../build-local ./smoke
 */

#include <stdio.h>
#include <string.h>

#include "agw/c_abi.h"

int main(void)
{
    agw_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.gateway_endpoint = "gw.example.test";
    cfg.agw_public_key_pem = "not a real pem key"; /* → 1105 без обращения к сети */
    cfg.request_timeout_msecs = 5000;

    agw_client *client = agw_client_create(&cfg);
    if (client == NULL) {
        printf("FAIL: agw_client_create returned NULL\n");
        return 1;
    }

    agw_response r = agw_client_post(client, "https://%1/api/v1/test", "{\"x\":1}", "", "", NULL);
    printf("post error code = %d\n", r.error);

    int ok = (r.error == 1105); /* ApiMissingAgwPublicKey */

    agw_response_free(&r);
    agw_client_destroy(client);

    printf(ok ? "OK\n" : "FAIL\n");
    return ok ? 0 : 1;
}
