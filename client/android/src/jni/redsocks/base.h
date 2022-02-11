#ifndef BASE_H_SUN_JUN__3_20_15_57_2007
#define BASE_H_SUN_JUN__3_20_15_57_2007

int getdestaddr(int fd, const struct sockaddr_in *client, const struct sockaddr_in *bindaddr, struct sockaddr_in *destaddr);
int apply_tcp_keepalive(int fd);

uint32_t redsocks_conn_max();
uint32_t connpres_idle_timeout();
uint32_t max_accept_backoff_ms();

/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
#endif /* BASE_H_SUN_JUN__3_20_15_57_2007 */
