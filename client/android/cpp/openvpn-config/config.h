
/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */


/* DISABLE PERSISTEN TUN */
#undef TUNSETPERSIST

/* Enable deferred authentication */
#define CONFIGURE_DEF_AUTH 1

/* Enable internal packet filter */
#undef CONFIGURE_PF //1

/* enable iproute2 support */
#undef CONFIG_FEATURE_IPROUTE

/* Use memory debugging function in OpenSSL */
/* #undef CRYPTO_MDEBUG */
#define HAVE_BASENAME
/* Use dmalloc memory debugging library */
/* #undef DMALLOC */

/* Dimension to use for empty array declaration */
#define EMPTY_ARRAY_SIZE 0

/* Enable client capability only */
#define ENABLE_CLIENT_ONLY 1

/* Enable debugging support */
#define ENABLE_DEBUG 1

/* Enable internal fragmentation support */
#define ENABLE_FRAGMENT 1

/* Enable HTTP proxy support */
#define ENABLE_HTTP_PROXY 1

/* Enable management server capability */
#define ENABLE_MANAGEMENT 1

/* Enable multi-homed UDP server capability */
#define ENABLE_MULTIHOME 0

/* Allow --askpass and --auth-user-pass passwords to be read from a file */
#define ENABLE_PASSWORD_SAVE 1

/* Enable TCP Server port sharing */
#define ENABLE_PORT_SHARE 1

/* Enable smaller executable size */
/* #undef ENABLE_SMALL */

/* Enable Socks proxy support */
#define ENABLE_SOCKS 1

/* Define to 1 if you have the `accept' function. */
#define HAVE_ACCEPT 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `bind' function. */
#define HAVE_BIND 1

/* Define to 1 if you have the `chdir' function. */
#define HAVE_CHDIR 1

/* Define to 1 if you have the `chroot' function. */
#define HAVE_CHROOT 1

/* Define to 1 if you have the `chsize' function. */
/* #undef HAVE_CHSIZE */

/* struct cmsghdr needed for extended socket error support */
#define HAVE_CMSGHDR 1

/* Define to 1 if you have the `connect' function. */
#define HAVE_CONNECT 1

/* Define to 1 if your compiler supports GNU GCC-style variadic macros */
#define HAVE_CPP_VARARG_MACRO_GCC 1

/* Define to 1 if your compiler supports ISO C99 variadic macros */
#define HAVE_CPP_VARARG_MACRO_ISO 1

/* Define to 1 if you have the `ctime' function. */
#define HAVE_CTIME 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the `daemon' function. */
#define HAVE_DAEMON 1

/* Define to 1 if you have the `dup' function. */
#define HAVE_DUP 1

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the `ENGINE_cleanup' function. */
#define HAVE_ENGINE_CLEANUP 0

/* Define to 1 if you have the `ENGINE_load_builtin_engines' function. */
#define HAVE_ENGINE_LOAD_BUILTIN_ENGINES 0

/* Define to 1 if you have the `ENGINE_register_all_complete' function. */
#define HAVE_ENGINE_REGISTER_ALL_COMPLETE 0

/* epoll_create function is defined */
#define HAVE_EPOLL_CREATE 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <err.h> header file. */
#define HAVE_ERR_H 1

/* Define to 1 if you have the `EVP_CIPHER_CTX_set_key_length' function. */
#define HAVE_EVP_CIPHER_CTX_SET_KEY_LENGTH 1

/* Define to 1 if you have the `execve' function. */
#define HAVE_EXECVE 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `flock' function. */
#define HAVE_FLOCK 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if you have the `getgrnam' function. */
#define HAVE_GETGRNAM 1

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the `getpass' function. */
/* #define HAVE_GETPASS 1 */

/* Define to 1 if you have the `getpeereid' function. */
/* #undef HAVE_GETPEEREID */

/* Define to 1 if you have the `getpeername' function. */
#define HAVE_GETPEERNAME 1

/* Define to 1 if you have the `getpid' function. */
#define HAVE_GETPID 1

/* Define to 1 if you have the `getpwnam' function. */
#define HAVE_GETPWNAM 1

/* Define to 1 if you have the `getsockname' function. */
#define HAVE_GETSOCKNAME 1

/* Define to 1 if you have the `getsockopt' function. */
#define HAVE_GETSOCKOPT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* struct in_pktinfo needed for IP_PKTINFO support */
#define HAVE_IN_PKTINFO 1

#define HAVE_IPI_SPEC_DST 1

/* struct iovec needed for IPv6 support */
#define HAVE_IOVEC 1

/* struct iphdr needed for IPv6 support */
#define HAVE_IPHDR 1

/* Define to 1 if you have the <linux/errqueue.h> header file. */
#define HAVE_LINUX_ERRQUEUE_H 1

/* Define to 1 if you have the <linux/if_tun.h> header file. */
#define HAVE_LINUX_IF_TUN_H 1

/* Define to 1 if you have the <linux/sockios.h> header file. */
#define HAVE_LINUX_SOCKIOS_H 1

/* Define to 1 if you have the <linux/types.h> header file. */
#define HAVE_LINUX_TYPES_H 1

/* Define to 1 if you have the `listen' function. */
#define HAVE_LISTEN 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mlockall' function. */
/* #define HAVE_MLOCKALL 1*/

/* struct msghdr needed for extended socket error support */
#define HAVE_MSGHDR 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
#define HAVE_NETINET_IF_ETHER_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
#define HAVE_NETINET_IN_SYSTM_H 1

/* Define to 1 if you have the <netinet/ip.h> header file. */
#define HAVE_NETINET_IP_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define to 1 if you have the <net/if_tun.h> header file. */
/* #undef HAVE_NET_IF_TUN_H */

/* Define to 1 if you have the <net/tun/if_tun.h> header file. */
/* #undef HAVE_NET_TUN_IF_TUN_H */

/* Define to 1 if you have the `nice' function. */
#define HAVE_NICE 1

/* Define to 1 if you have the `openlog' function. */
#define HAVE_OPENLOG 1

/* Define to 1 if you have the <openssl/engine.h> header file. */
#define HAVE_OPENSSL_ENGINE_H 0

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the `putenv' function. */
#define HAVE_PUTENV 1

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `readv' function. */
#define HAVE_READV 1

/* Define to 1 if you have the `recv' function. */
#define HAVE_RECV 1

/* Define to 1 if you have the `recvfrom' function. */
#define HAVE_RECVFROM 1

/* Define to 1 if you have the `recvmsg' function. */
#define HAVE_RECVMSG 1

/* Define to 1 if you have the <resolv.h> header file. */
#define HAVE_RESOLV_H 1

/* Indicates if res_init is available */
#define HAVE_RES_INIT 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `send' function. */
#define HAVE_SEND 1

/* Define to 1 if you have the `sendmsg' function. */
#define HAVE_SENDMSG 1

/* Define to 1 if you have the `sendto' function. */
#define HAVE_SENDTO 1

/* SELinux support */
/* #undef HAVE_SETCON */

/* Define to 1 if you have the `setgid' function. */
#define HAVE_SETGID 1

/* Define to 1 if you have the `setgroups' function. */
#define HAVE_SETGROUPS 1

/* Define to 1 if you have the `setsid' function. */
#define HAVE_SETSID 1

/* Define to 1 if you have the `setsockopt' function. */
#define HAVE_SETSOCKOPT 1

/* Define to 1 if you have the `setuid' function. */
#define HAVE_SETUID 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* struct sock_extended_err needed for extended socket error support */
#define HAVE_SOCK_EXTENDED_ERR 1

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <stropts.h> header file. */
#define HAVE_STROPTS_H 1

/* Define to 1 if you have the `syslog' function. */
 #define HAVE_SYSLOG 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the `system' function. */
#define HAVE_SYSTEM 1

/* Define to 1 if you have the <sys/epoll.h> header file. */
#define HAVE_SYS_EPOLL_H 1

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `time' function. */
#define HAVE_TIME 1

/* struct tun_pi needed for IPv6 support */
#define HAVE_TUN_PI 1

/* Define to 1 if you have the `umask' function. */
#define HAVE_UMASK 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unlink' function. */
#define HAVE_UNLINK 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the `writev' function. */
#define HAVE_WRITEV 1

/* Path to ifconfig tool */
#define IFCONFIG_PATH "/system/xbin/ifconfig"

/* Path to iproute tool */
#define IPROUTE_PATH "ip"

/* Use lzo/ directory prefix for LZO header files (for LZO 2.0) */
#define LZO_HEADER_DIR 1

/* LZO version number */
#define LZO_VERSION_NUM "2"

/* Name of package */
#define PACKAGE "openvpn"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "openvpn-users@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "OpenVPN"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "OpenVPN 2.6-icsopenvpn"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "openvpn"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.6_master"

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Path to route tool */
#define ROUTE_PATH "/system/xbin/route"

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable strict options check between peers */
/* #undef STRICT_OPTIONS_CHECK */

/* The TAP-Win32 id defined in tap-win32/SOURCES */
#define TAP_ID "tap0901"

/* The TAP-Win32 version number is defined in tap-win32/SOURCES */
#define TAP_WIN32_MIN_MAJOR 9

/* The TAP-Win32 version number is defined in tap-win32/SOURCES */
#define TAP_WIN32_MIN_MINOR 1

/* A string representing our target */
#define TARGET_ALIAS TARGET_ABI

/* Are we running on Mac OS X? */
/* #undef TARGET_DARWIN */

/* Are we running on DragonFlyBSD? */
/* #undef TARGET_DRAGONFLY */

/* Are we running on FreeBSD? */
/* #undef TARGET_FREEBSD */

/* Are we running on Android Linux? */
/* TARGET_LINUX is not enable since the TARGET_XXX options mainly
   control different tun/tap, ifconfig behaviour and Android VpnProvider
   is very different from Linux */
#define TARGET_ANDROID

/* Are we running on Linux? */
/* #define TARGET_LINUX 1 */

/* Are we running NetBSD? */
/* #undef TARGET_NETBSD */

/* Are we running on OpenBSD? */
/* #undef TARGET_OPENBSD */

/* Are we running on Solaris? */
/* #undef TARGET_SOLARIS */

/* Are we running WIN32? */
/* #undef TARGET_WIN32 */

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Win32 builtin */
/* #undef UF */

/* Use OpenSSL crypto library */
#define USE_CRYPTO 1

/* Use libdl for dynamic library loading */
#define USE_LIBDL 1

/* Use LoadLibrary to load DLLs on Windows */
/* #undef USE_LOAD_LIBRARY */

/* Use LZO compression library */
#define ENABLE_LZO 1
#define ENABLE_SNAPPY 1
#define ENABLE_LZ4 1
#define NEED_COMPAT_LZ4 1

/* Enable PKCS11 capability */
/* #undef USE_PKCS11 */

/* Use pthread-based multithreading */
/* #undef USE_PTHREAD */

/* Use OpenSSL SSL library */
#define ENABLE_SSL 1
#define USE_SSL 1
#define ENABLE_CRYPTO 1

/* via android.mk */
/*#define ENABLE_CRYPTO_OPENSSL 1*/
/* #define ENABLE_CRYPTO_POLARSSL 1 */

/* Use valgrind memory debugging library */
/* #undef USE_VALGRIND */

/* Version number of package */
//#define VERSION "2.3.1"

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Some systems don't define in_addr_t */
/* #undef in_addr_t */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* type to use in place of socklen_t if not defined */
/* #undef socklen_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* 16-bit unsigned type */
/* #undef uint16_t */

/* 32-bit unsigned type */
/* #undef uint32_t */

/* 8-bit unsigned type */
/* #undef uint8_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

// New version
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_LZO_LZOUTIL_H 1
#define HAVE_LZO_LZO1X_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_TIME_H 1
// #define HAVE_CONFIG_VERSION_H 1
#define PATH_SEPARATOR_STR "/"
#define HAVE_SA_FAMILY_T 1


// Workaround for bionc
#define IPPROTO_IP IPPROTO_IP
#define IPPROTO_TCP IPPROTO_TCP

int res_init();

#define HAVE_AEAD_CIPHER_MODES 1

#define HAVE_EVP_MD_CTX_RESET 1
#define HAVE_EVP_MD_CTX_FREE 1
#define HAVE_EVP_MD_CTX_NEW 1
#define HAVE_EVP_CIPHER_CTX_FREE 1
#define HAVE_EVP_CIPHER_CTX_NEW 1
#define HAVE_HMAC_CTX_RESET 1
#define HAVE_HMAC_CTX_FREE 1
#define HAVE_HMAC_CTX_NEW 1
#define HAVE_SSL_CTX_GET_DEFAULT_PASSWD_CB_USERDATA 1
#define HAVE_SSL_CTX_GET_DEFAULT_PASSWD_CB 1
#define HAVE_X509_GET0_PUBKEY 1
#define HAVE_X509_STORE_GET0_OBJECTS 1
#define HAVE_X509_OBJECT_FREE 1
#define HAVE_X509_OBJECT_GET_TYPE 1
#define HAVE_EVP_PKEY_GET0_RSA 1
#define HAVE_EVP_PKEY_ID 1
#define HAVE_EVP_PKEY_GET0_DSA 1
#define HAVE_RSA_SET_FLAGS 1
#define HAVE_RSA_GET0_KEY 1
#define HAVE_RSA_SET0_KEY 1
#define HAVE_RSA_BITS 1
#define HAVE_DSA_BITS 1
#define HAVE_DSA_GET0_PQG 1
#define HAVE_RSA_METH_NEW 1
#define HAVE_RSA_METH_FREE 1
#define HAVE_RSA_METH_SET_PUB_ENC 1
#define HAVE_RSA_METH_SET_PUB_DEC 1
#define HAVE_RSA_METH_SET_PRIV_DEC 1
#define HAVE_RSA_METH_SET_PRIV_ENC 1
#define HAVE_RSA_METH_SET_INIT 1
#define HAVE_RSA_METH_SET_FINISH 1
#define HAVE_RSA_METH_SET0_APP_DATA 1
#define HAVE_EVP_PKEY_GET0_EC_KEY 1
#define HAVE_EC_GROUP_ORDER_BITS 1
#define HAVE_RSA_METH_GET0_APP_DATA 1
#define HAVE_RSA_METH_SET_SIGN 1

#define ENABLE_OFB_CFB_MODE 1

#define HAVE_X509_GET0_NOTBEFORE 1
#define HAVE_X509_GET0_NOTAFTER 1

#define HAVE_OPENSSL_VERSION 1
#define HAVE_EVP_CIPHER_CTX_RESET


#define _SOCKLEN_T_DECLARED 1
