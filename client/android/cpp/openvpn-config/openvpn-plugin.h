/* include/openvpn-plugin.h.  Generated from openvpn-plugin.h.in by configure.  */
/*
 *  OpenVPN -- An application to securely tunnel IP networks
 *             over a single TCP/UDP port, with support for SSL/TLS-based
 *             session authentication and key exchange,
 *             packet encryption, packet authentication, and
 *             packet compression.
 *
 *  Copyright (C) 2002-2018 OpenVPN Inc <sales@openvpn.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef OPENVPN_PLUGIN_H_
#define OPENVPN_PLUGIN_H_

#define OPENVPN_PLUGIN_VERSION 3

#ifdef ENABLE_CRYPTO_MBEDTLS
#include <mbedtls/x509_crt.h>
#ifndef __OPENVPN_X509_CERT_T_DECLARED
#define __OPENVPN_X509_CERT_T_DECLARED
typedef mbedtls_x509_crt openvpn_x509_cert_t;
#endif
#else  /* ifdef ENABLE_CRYPTO_MBEDTLS */
#include <openssl/x509.h>
#ifndef __OPENVPN_X509_CERT_T_DECLARED
#define __OPENVPN_X509_CERT_T_DECLARED
typedef X509 openvpn_x509_cert_t;
#endif
#endif

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Provide some basic version information to plug-ins at OpenVPN compile time
 * This is will not be the complete version
 */
#define OPENVPN_VERSION_MAJOR 2
#define OPENVPN_VERSION_MINOR 5
#define OPENVPN_VERSION_PATCH "_git"

/*
 * Plug-in types.  These types correspond to the set of script callbacks
 * supported by OpenVPN.
 *
 * This is the general call sequence to expect when running in server mode:
 *
 * Initial Server Startup:
 *
 * FUNC: openvpn_plugin_open_v1
 * FUNC: openvpn_plugin_client_constructor_v1 (this is the top-level "generic"
 *                                             client template)
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_UP
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_ROUTE_UP
 *
 * New Client Connection:
 *
 * FUNC: openvpn_plugin_client_constructor_v1
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_ENABLE_PF
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_TLS_VERIFY (called once for every cert
 *                                                     in the server chain)
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_TLS_FINAL
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_IPCHANGE
 *
 * [If OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY returned OPENVPN_PLUGIN_FUNC_DEFERRED,
 * we don't proceed until authentication is verified via auth_control_file]
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_CLIENT_CONNECT_V2
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_LEARN_ADDRESS
 *
 * [Client session ensues]
 *
 * For each "TLS soft reset", according to reneg-sec option (or similar):
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_ENABLE_PF
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_TLS_VERIFY (called once for every cert
 *                                                     in the server chain)
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_TLS_FINAL
 *
 * [If OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY returned OPENVPN_PLUGIN_FUNC_DEFERRED,
 * we expect that authentication is verified via auth_control_file within
 * the number of seconds defined by the "hand-window" option.  Data channel traffic
 * will continue to flow uninterrupted during this period.]
 *
 * [Client session continues]
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_CLIENT_DISCONNECT
 * FUNC: openvpn_plugin_client_destructor_v1
 *
 * [ some time may pass ]
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_LEARN_ADDRESS (this coincides with a
 *                                                            lazy free of initial
 *                                                            learned addr object)
 * Server Shutdown:
 *
 * FUNC: openvpn_plugin_func_v1 OPENVPN_PLUGIN_DOWN
 * FUNC: openvpn_plugin_client_destructor_v1 (top-level "generic" client)
 * FUNC: openvpn_plugin_close_v1
 */
#define OPENVPN_PLUGIN_UP                        0
#define OPENVPN_PLUGIN_DOWN                      1
#define OPENVPN_PLUGIN_ROUTE_UP                  2
#define OPENVPN_PLUGIN_IPCHANGE                  3
#define OPENVPN_PLUGIN_TLS_VERIFY                4
#define OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY     5
#define OPENVPN_PLUGIN_CLIENT_CONNECT            6
#define OPENVPN_PLUGIN_CLIENT_DISCONNECT         7
#define OPENVPN_PLUGIN_LEARN_ADDRESS             8
#define OPENVPN_PLUGIN_CLIENT_CONNECT_V2         9
#define OPENVPN_PLUGIN_TLS_FINAL                10
#define OPENVPN_PLUGIN_ENABLE_PF                11
#define OPENVPN_PLUGIN_ROUTE_PREDOWN            12
#define OPENVPN_PLUGIN_CLIENT_CONNECT_DEFER     13
#define OPENVPN_PLUGIN_CLIENT_CONNECT_DEFER_V2  14
#define OPENVPN_PLUGIN_CLIENT_CRRESPONSE        15
#define OPENVPN_PLUGIN_N                        16

/*
 * Build a mask out of a set of plug-in types.
 */
#define OPENVPN_PLUGIN_MASK(x) (1<<(x))

/*
 * A pointer to a plugin-defined object which contains
 * the object state.
 */
typedef void *openvpn_plugin_handle_t;

/*
 * Return value for openvpn_plugin_func_v1 function
 */
#define OPENVPN_PLUGIN_FUNC_SUCCESS  0
#define OPENVPN_PLUGIN_FUNC_ERROR    1
#define OPENVPN_PLUGIN_FUNC_DEFERRED 2

/*
 * For Windows (needs to be modified for MSVC)
 */
#if defined(_WIN32) && !defined(OPENVPN_PLUGIN_H)
#define OPENVPN_EXPORT __declspec(dllexport)
#else
#define OPENVPN_EXPORT
#endif

/*
 * If OPENVPN_PLUGIN_H is defined, we know that we are being
 * included in an OpenVPN compile, rather than a plugin compile.
 */
#ifdef OPENVPN_PLUGIN_H

/*
 * We are compiling OpenVPN.
 */
#define OPENVPN_PLUGIN_DEF        typedef
#define OPENVPN_PLUGIN_FUNC(name) (*name)

#else  /* ifdef OPENVPN_PLUGIN_H */

/*
 * We are compiling plugin.
 */
#define OPENVPN_PLUGIN_DEF        OPENVPN_EXPORT
#define OPENVPN_PLUGIN_FUNC(name) name

#endif

/*
 * Used by openvpn_plugin_func to return structured
 * data.  The plugin should allocate all structure
 * instances, name strings, and value strings with
 * malloc, since OpenVPN will assume that it
 * can free the list by calling free() over the same.
 */
struct openvpn_plugin_string_list
{
    struct openvpn_plugin_string_list *next;
    char *name;
    char *value;
};


/* openvpn_plugin_{open,func}_v3() related structs */

/**
 * Defines version of the v3 plugin argument structs
 *
 * Whenever one or more of these structs are modified, this constant
 * must be updated.  A changelog should be appended in this comment
 * as well, to make it easier to see what information is available
 * in the different versions.
 *
 * Version   Comment
 *    1      Initial plugin v3 structures providing the same API as
 *           the v2 plugin interface, X509 certificate information +
 *           a logging API for plug-ins.
 *
 *    2      Added ssl_api member in struct openvpn_plugin_args_open_in
 *           which identifies the SSL implementation OpenVPN is compiled
 *           against.
 *
 *    3      Added ovpn_version, ovpn_version_major, ovpn_version_minor
 *           and ovpn_version_patch to provide the runtime version of
 *           OpenVPN to plug-ins.
 *
 *    4      Exported secure_memzero() as plugin_secure_memzero()
 *
 *    5      Exported openvpn_base64_encode() as plugin_base64_encode()
 *           Exported openvpn_base64_decode() as plugin_base64_decode()
 */
#define OPENVPN_PLUGINv3_STRUCTVER 5

/**
 * Definitions needed for the plug-in callback functions.
 */
typedef enum
{
    PLOG_ERR    = (1 << 0),/* Error condition message */
    PLOG_WARN   = (1 << 1),/* General warning message */
    PLOG_NOTE   = (1 << 2),/* Informational message */
    PLOG_DEBUG  = (1 << 3),/* Debug message, displayed if verb >= 7 */

    PLOG_ERRNO  = (1 << 8),/* Add error description to message */
    PLOG_NOMUTE = (1 << 9), /* Mute setting does not apply for message */

} openvpn_plugin_log_flags_t;


#ifdef __GNUC__
#if __USE_MINGW_ANSI_STDIO
#define _ovpn_chk_fmt(a, b) __attribute__ ((format(gnu_printf, (a), (b))))
#else
#define _ovpn_chk_fmt(a, b) __attribute__ ((format(__printf__, (a), (b))))
#endif
#else  /* ifdef __GNUC__ */
#define _ovpn_chk_fmt(a, b)
#endif

typedef void (*plugin_log_t)(openvpn_plugin_log_flags_t flags,
                             const char *plugin_name,
                             const char *format, ...) _ovpn_chk_fmt (3, 4);

typedef void (*plugin_vlog_t)(openvpn_plugin_log_flags_t flags,
                              const char *plugin_name,
                              const char *format,
                              va_list arglist) _ovpn_chk_fmt (3, 0);
/* #undef _ovpn_chk_fmt */

/**
 *  Export of secure_memzero() to be used inside plug-ins
 *
 *  @param data   Pointer to data to zeroise
 *  @param len    Length of data, in bytes
 *
 */
typedef void (*plugin_secure_memzero_t)(void *data, size_t len);

/**
 *  Export of openvpn_base64_encode() to be used inside plug-ins
 *
 *  @param data   Pointer to data to BASE64 encode
 *  @param size   Length of data, in bytes
 *  @param *str   Pointer to the return buffer.  This needed memory is
 *                allocated by openvpn_base64_encode() and needs to be free()d
 *                after use.
 *
 *  @return int   Returns the length of the buffer created, or -1 on error.
 *
 */
typedef int (*plugin_base64_encode_t)(const void *data, int size, char **str);

/**
 *  Export of openvpn_base64_decode() to be used inside plug-ins
 *
 *  @param str    Pointer to the BASE64 encoded data
 *  @param data   Pointer to the buffer where save the decoded data
 *  @param size   Size of the destination buffer
 *
 *  @return int   Returns the length of the decoded data, or -1 on error or
 *                if the destination buffer is too small.
 *
 */
typedef int (*plugin_base64_decode_t)(const char *str, void *data, int size);


/**
 * Used by the openvpn_plugin_open_v3() function to pass callback
 * function pointers to the plug-in.
 *
 * plugin_log
 * plugin_vlog : Use these functions to add information to the OpenVPN log file.
 *               Messages will only be displayed if the plugin_name parameter
 *               is set. PLOG_DEBUG messages will only be displayed with plug-in
 *               debug log verbosity (at the time of writing that's verb >= 7).
 *
 * plugin_secure_memzero
 *             : Use this function to securely wipe sensitive information from
 *               memory.  This function is declared in a way that the compiler
 *               will not remove these function calls during the compiler
 *               optimization phase.
 */
struct openvpn_plugin_callbacks
{
    plugin_log_t plugin_log;
    plugin_vlog_t plugin_vlog;
    plugin_secure_memzero_t plugin_secure_memzero;
    plugin_base64_encode_t plugin_base64_encode;
    plugin_base64_decode_t plugin_base64_decode;
};

/**
 * Used by the openvpn_plugin_open_v3() function to indicate to the
 * plug-in what kind of SSL implementation OpenVPN uses.  This is
 * to avoid SEGV issues when OpenVPN is complied against mbed TLS
 * and the plug-in against OpenSSL.
 */
typedef enum {
    SSLAPI_NONE,
    SSLAPI_OPENSSL,
    SSLAPI_MBEDTLS
} ovpnSSLAPI;

/**
 * Arguments used to transport variables to the plug-in.
 * The struct openvpn_plugin_args_open_in is only used
 * by the openvpn_plugin_open_v3() function.
 *
 * STRUCT MEMBERS
 *
 * type_mask : Set by OpenVPN to the logical OR of all script
 *             types which this version of OpenVPN supports.
 *
 * argv : a NULL-terminated array of options provided to the OpenVPN
 *        "plug-in" directive.  argv[0] is the dynamic library pathname.
 *
 * envp : a NULL-terminated array of OpenVPN-set environmental
 *        variables in "name=value" format.  Note that for security reasons,
 *        these variables are not actually written to the "official"
 *        environmental variable store of the process.
 *
 * callbacks : a pointer to the plug-in callback function struct.
 *
 */
struct openvpn_plugin_args_open_in
{
    const int type_mask;
    const char **const argv;
    const char **const envp;
    struct openvpn_plugin_callbacks *callbacks;
    const ovpnSSLAPI ssl_api;
    const char *ovpn_version;
    const unsigned int ovpn_version_major;
    const unsigned int ovpn_version_minor;
    const char *const ovpn_version_patch;
};


/**
 * Arguments used to transport variables from the plug-in back
 * to the OpenVPN process.  The struct openvpn_plugin_args_open_return
 * is only used by the openvpn_plugin_open_v3() function.
 *
 * STRUCT MEMBERS
 *
 * type_mask  : The plug-in should set this value to the logical OR of all script
 *              types which the plug-in wants to intercept.  For example, if the
 *              script wants to intercept the client-connect and client-disconnect
 *              script types:
 *
 *              type_mask = OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_CONNECT)
 *                         | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_DISCONNECT)
 *
 * handle :     Pointer to a global plug-in context, created by the plug-in.  This pointer
 *              is passed on to the other plug-in calls.
 *
 * return_list : used to return data back to OpenVPN.
 *
 */
struct openvpn_plugin_args_open_return
{
    int type_mask;
    openvpn_plugin_handle_t handle;
    struct openvpn_plugin_string_list **return_list;
};

/**
 * Arguments used to transport variables to and from the
 * plug-in.  The struct openvpn_plugin_args_func is only used
 * by the openvpn_plugin_func_v3() function.
 *
 * STRUCT MEMBERS:
 *
 * type : one of the PLUGIN_x types.
 *
 * argv : a NULL-terminated array of "command line" options which
 *        would normally be passed to the script.  argv[0] is the dynamic
 *        library pathname.
 *
 * envp : a NULL-terminated array of OpenVPN-set environmental
 *        variables in "name=value" format.  Note that for security reasons,
 *        these variables are not actually written to the "official"
 *        environmental variable store of the process.
 *
 * handle : Pointer to a global plug-in context, created by the plug-in's openvpn_plugin_open_v3().
 *
 * per_client_context : the per-client context pointer which was returned by
 *        openvpn_plugin_client_constructor_v1, if defined.
 *
 * current_cert_depth : Certificate depth of the certificate being passed over
 *
 * *current_cert : X509 Certificate object received from the client
 *
 */
struct openvpn_plugin_args_func_in
{
    const int type;
    const char **const argv;
    const char **const envp;
    openvpn_plugin_handle_t handle;
    void *per_client_context;
    int current_cert_depth;
    openvpn_x509_cert_t *current_cert;
};


/**
 * Arguments used to transport variables to and from the
 * plug-in.  The struct openvpn_plugin_args_func is only used
 * by the openvpn_plugin_func_v3() function.
 *
 * STRUCT MEMBERS:
 *
 * return_list : used to return data back to OpenVPN for further processing/usage by
 *               the OpenVPN executable.
 *
 */
struct openvpn_plugin_args_func_return
{
    struct openvpn_plugin_string_list **return_list;
};

/*
 * Multiple plugin modules can be cascaded, and modules can be
 * used in tandem with scripts.  The order of operation is that
 * the module func() functions are called in the order that
 * the modules were specified in the config file.  If a script
 * was specified as well, it will be called last.  If the
 * return code of the module/script controls an authentication
 * function (such as tls-verify or auth-user-pass-verify), then
 * every module and script must return success (0) in order for
 * the connection to be authenticated.
 *
 * Notes:
 *
 * Plugins which use a privilege-separation model (by forking in
 * their initialization function before the main OpenVPN process
 * downgrades root privileges and/or executes a chroot) must
 * daemonize after a fork if the "daemon" environmental variable is
 * set.  In addition, if the "daemon_log_redirect" variable is set,
 * the plugin should preserve stdout/stderr across the daemon()
 * syscall.  See the daemonize() function in plugin/auth-pam/auth-pam.c
 * for an example.
 */

/*
 * Prototypes for functions which OpenVPN plug-ins must define.
 */

/*
 * FUNCTION: openvpn_plugin_open_v2
 *
 * REQUIRED: YES
 *
 * Called on initial plug-in load.  OpenVPN will preserve plug-in state
 * across SIGUSR1 restarts but not across SIGHUP restarts.  A SIGHUP reset
 * will cause the plugin to be closed and reopened.
 *
 * ARGUMENTS
 *
 * *type_mask : Set by OpenVPN to the logical OR of all script
 *              types which this version of OpenVPN supports.  The plug-in
 *              should set this value to the logical OR of all script types
 *              which the plug-in wants to intercept.  For example, if the
 *              script wants to intercept the client-connect and
 *              client-disconnect script types:
 *
 *              *type_mask = OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_CONNECT)
 *                         | OPENVPN_PLUGIN_MASK(OPENVPN_PLUGIN_CLIENT_DISCONNECT)
 *
 * argv : a NULL-terminated array of options provided to the OpenVPN
 *        "plug-in" directive.  argv[0] is the dynamic library pathname.
 *
 * envp : a NULL-terminated array of OpenVPN-set environmental
 *        variables in "name=value" format.  Note that for security reasons,
 *        these variables are not actually written to the "official"
 *        environmental variable store of the process.
 *
 * return_list : used to return data back to OpenVPN.
 *
 * RETURN VALUE
 *
 * An openvpn_plugin_handle_t value on success, NULL on failure
 */
OPENVPN_PLUGIN_DEF openvpn_plugin_handle_t OPENVPN_PLUGIN_FUNC(openvpn_plugin_open_v2)
    (unsigned int *type_mask,
    const char *argv[],
    const char *envp[],
    struct openvpn_plugin_string_list **return_list);

/*
 * FUNCTION: openvpn_plugin_func_v2
 *
 * Called to perform the work of a given script type.
 *
 * REQUIRED: YES
 *
 * ARGUMENTS
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * type : one of the PLUGIN_x types
 *
 * argv : a NULL-terminated array of "command line" options which
 *        would normally be passed to the script.  argv[0] is the dynamic
 *        library pathname.
 *
 * envp : a NULL-terminated array of OpenVPN-set environmental
 *        variables in "name=value" format.  Note that for security reasons,
 *        these variables are not actually written to the "official"
 *        environmental variable store of the process.
 *
 * per_client_context : the per-client context pointer which was returned by
 *        openvpn_plugin_client_constructor_v1, if defined.
 *
 * return_list : used to return data back to OpenVPN.
 *
 * RETURN VALUE
 *
 * OPENVPN_PLUGIN_FUNC_SUCCESS on success, OPENVPN_PLUGIN_FUNC_ERROR on failure
 *
 * In addition, OPENVPN_PLUGIN_FUNC_DEFERRED may be returned by
 * OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY.  This enables asynchronous
 * authentication where the plugin (or one of its agents) may indicate
 * authentication success/failure some number of seconds after the return
 * of the OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY handler by writing a single
 * char to the file named by auth_control_file in the environmental variable
 * list (envp).
 *
 * first char of auth_control_file:
 * '0' -- indicates auth failure
 * '1' -- indicates auth success
 *
 * OpenVPN will delete the auth_control_file after it goes out of scope.
 *
 * If an OPENVPN_PLUGIN_ENABLE_PF handler is defined and returns success
 * for a particular client instance, packet filtering will be enabled for that
 * instance.  OpenVPN will then attempt to read the packet filter configuration
 * from the temporary file named by the environmental variable pf_file.  This
 * file may be generated asynchronously and may be dynamically updated during the
 * client session, however the client will be blocked from sending or receiving
 * VPN tunnel packets until the packet filter file has been generated.  OpenVPN
 * will periodically test the packet filter file over the life of the client
 * instance and reload when modified.  OpenVPN will delete the packet filter file
 * when the client instance goes out of scope.
 *
 * Packet filter file grammar:
 *
 * [CLIENTS DROP|ACCEPT]
 * {+|-}common_name1
 * {+|-}common_name2
 * . . .
 * [SUBNETS DROP|ACCEPT]
 * {+|-}subnet1
 * {+|-}subnet2
 * . . .
 * [END]
 *
 * Subnet: IP-ADDRESS | IP-ADDRESS/NUM_NETWORK_BITS
 *
 * CLIENTS refers to the set of clients (by their common-name) which
 * this instance is allowed ('+') to connect to, or is excluded ('-')
 * from connecting to.  Note that in the case of client-to-client
 * connections, such communication must be allowed by the packet filter
 * configuration files of both clients.
 *
 * SUBNETS refers to IP addresses or IP address subnets which this
 * instance may connect to ('+') or is excluded ('-') from connecting
 * to.
 *
 * DROP or ACCEPT defines default policy when there is no explicit match
 * for a common-name or subnet.  The [END] tag must exist.  A special
 * purpose tag called [KILL] will immediately kill the client instance.
 * A given client or subnet rule applies to both incoming and outgoing
 * packets.
 *
 * See plugin/defer/simple.c for an example on using asynchronous
 * authentication and client-specific packet filtering.
 */
OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_func_v2)
    (openvpn_plugin_handle_t handle,
    const int type,
    const char *argv[],
    const char *envp[],
    void *per_client_context,
    struct openvpn_plugin_string_list **return_list);


/*
 * FUNCTION: openvpn_plugin_open_v3
 *
 * REQUIRED: YES
 *
 * Called on initial plug-in load.  OpenVPN will preserve plug-in state
 * across SIGUSR1 restarts but not across SIGHUP restarts.  A SIGHUP reset
 * will cause the plugin to be closed and reopened.
 *
 * ARGUMENTS
 *
 * version : fixed value, defines the API version of the OpenVPN plug-in API.  The plug-in
 *           should validate that this value is matching the OPENVPN_PLUGINv3_STRUCTVER
 *           value.
 *
 * arguments : Structure with all arguments available to the plug-in.
 *
 * retptr :    used to return data back to OpenVPN.
 *
 * RETURN VALUE
 *
 * OPENVPN_PLUGIN_FUNC_SUCCESS on success, OPENVPN_PLUGIN_FUNC_ERROR on failure
 */
OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_open_v3)
    (const int version,
    struct openvpn_plugin_args_open_in const *arguments,
    struct openvpn_plugin_args_open_return *retptr);

/*
 * FUNCTION: openvpn_plugin_func_v3
 *
 * Called to perform the work of a given script type.
 *
 * REQUIRED: YES
 *
 * ARGUMENTS
 *
 * version : fixed value, defines the API version of the OpenVPN plug-in API.  The plug-in
 *           should validate that this value is matching the OPENVPN_PLUGIN_VERSION value.
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * return_list : used to return data back to OpenVPN.
 *
 * RETURN VALUE
 *
 * OPENVPN_PLUGIN_FUNC_SUCCESS on success, OPENVPN_PLUGIN_FUNC_ERROR on failure
 *
 * In addition, OPENVPN_PLUGIN_FUNC_DEFERRED may be returned by
 * OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY.  This enables asynchronous
 * authentication where the plugin (or one of its agents) may indicate
 * authentication success/failure some number of seconds after the return
 * of the OPENVPN_PLUGIN_AUTH_USER_PASS_VERIFY handler by writing a single
 * char to the file named by auth_control_file in the environmental variable
 * list (envp).
 *
 * first char of auth_control_file:
 * '0' -- indicates auth failure
 * '1' -- indicates auth success
 *
 * OpenVPN will delete the auth_control_file after it goes out of scope.
 *
 * If an OPENVPN_PLUGIN_ENABLE_PF handler is defined and returns success
 * for a particular client instance, packet filtering will be enabled for that
 * instance.  OpenVPN will then attempt to read the packet filter configuration
 * from the temporary file named by the environmental variable pf_file.  This
 * file may be generated asynchronously and may be dynamically updated during the
 * client session, however the client will be blocked from sending or receiving
 * VPN tunnel packets until the packet filter file has been generated.  OpenVPN
 * will periodically test the packet filter file over the life of the client
 * instance and reload when modified.  OpenVPN will delete the packet filter file
 * when the client instance goes out of scope.
 *
 * Packet filter file grammar:
 *
 * [CLIENTS DROP|ACCEPT]
 * {+|-}common_name1
 * {+|-}common_name2
 * . . .
 * [SUBNETS DROP|ACCEPT]
 * {+|-}subnet1
 * {+|-}subnet2
 * . . .
 * [END]
 *
 * Subnet: IP-ADDRESS | IP-ADDRESS/NUM_NETWORK_BITS
 *
 * CLIENTS refers to the set of clients (by their common-name) which
 * this instance is allowed ('+') to connect to, or is excluded ('-')
 * from connecting to.  Note that in the case of client-to-client
 * connections, such communication must be allowed by the packet filter
 * configuration files of both clients.
 *
 * SUBNETS refers to IP addresses or IP address subnets which this
 * instance may connect to ('+') or is excluded ('-') from connecting
 * to.
 *
 * DROP or ACCEPT defines default policy when there is no explicit match
 * for a common-name or subnet.  The [END] tag must exist.  A special
 * purpose tag called [KILL] will immediately kill the client instance.
 * A given client or subnet rule applies to both incoming and outgoing
 * packets.
 *
 * See plugin/defer/simple.c for an example on using asynchronous
 * authentication and client-specific packet filtering.
 */
OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_func_v3)
    (const int version,
    struct openvpn_plugin_args_func_in const *arguments,
    struct openvpn_plugin_args_func_return *retptr);

/*
 * FUNCTION: openvpn_plugin_close_v1
 *
 * REQUIRED: YES
 *
 * ARGUMENTS
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * Called immediately prior to plug-in unload.
 */
OPENVPN_PLUGIN_DEF void OPENVPN_PLUGIN_FUNC(openvpn_plugin_close_v1)
    (openvpn_plugin_handle_t handle);

/*
 * FUNCTION: openvpn_plugin_abort_v1
 *
 * REQUIRED: NO
 *
 * ARGUMENTS
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * Called when OpenVPN is in the process of aborting due to a fatal error.
 * Will only be called on an open context returned by a prior successful
 * openvpn_plugin_open callback.
 */
OPENVPN_PLUGIN_DEF void OPENVPN_PLUGIN_FUNC(openvpn_plugin_abort_v1)
    (openvpn_plugin_handle_t handle);

/*
 * FUNCTION: openvpn_plugin_client_constructor_v1
 *
 * Called to allocate a per-client memory region, which
 * is then passed to the openvpn_plugin_func_v2 function.
 * This function is called every time the OpenVPN server
 * constructs a client instance object, which normally
 * occurs when a session-initiating packet is received
 * by a new client, even before the client has authenticated.
 *
 * This function should allocate the private memory needed
 * by the plugin to track individual OpenVPN clients, and
 * return a void * to this memory region.
 *
 * REQUIRED: NO
 *
 * ARGUMENTS
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * RETURN VALUE
 *
 * void * pointer to plugin's private per-client memory region, or NULL
 * if no memory region is required.
 */
OPENVPN_PLUGIN_DEF void *OPENVPN_PLUGIN_FUNC(openvpn_plugin_client_constructor_v1)
    (openvpn_plugin_handle_t handle);

/*
 * FUNCTION: openvpn_plugin_client_destructor_v1
 *
 * This function is called on client instance object destruction.
 *
 * REQUIRED: NO
 *
 * ARGUMENTS
 *
 * handle : the openvpn_plugin_handle_t value which was returned by
 *          openvpn_plugin_open.
 *
 * per_client_context : the per-client context pointer which was returned by
 *        openvpn_plugin_client_constructor_v1, if defined.
 */
OPENVPN_PLUGIN_DEF void OPENVPN_PLUGIN_FUNC(openvpn_plugin_client_destructor_v1)
    (openvpn_plugin_handle_t handle, void *per_client_context);

/*
 * FUNCTION: openvpn_plugin_select_initialization_point_v1
 *
 * Several different points exist in OpenVPN's initialization sequence where
 * the openvpn_plugin_open function can be called.  While the default is
 * OPENVPN_PLUGIN_INIT_PRE_DAEMON, this function can be used to select a
 * different initialization point.  For example, if your plugin needs to
 * return configuration parameters to OpenVPN, use
 * OPENVPN_PLUGIN_INIT_PRE_CONFIG_PARSE.
 *
 * REQUIRED: NO
 *
 * RETURN VALUE:
 *
 * An OPENVPN_PLUGIN_INIT_x value.
 */
#define OPENVPN_PLUGIN_INIT_PRE_CONFIG_PARSE 1
#define OPENVPN_PLUGIN_INIT_PRE_DAEMON       2 /* default */
#define OPENVPN_PLUGIN_INIT_POST_DAEMON      3
#define OPENVPN_PLUGIN_INIT_POST_UID_CHANGE  4

OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_select_initialization_point_v1)
    (void);

/*
 * FUNCTION: openvpn_plugin_min_version_required_v1
 *
 * This function is called by OpenVPN to query the minimum
 * plugin interface version number required by the plugin.
 *
 * REQUIRED: NO
 *
 * RETURN VALUE
 *
 * The minimum OpenVPN plugin interface version number necessary to support
 * this plugin.
 */
OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_min_version_required_v1)
    (void);

/*
 * Deprecated functions which are still supported for backward compatibility.
 */

OPENVPN_PLUGIN_DEF openvpn_plugin_handle_t OPENVPN_PLUGIN_FUNC(openvpn_plugin_open_v1)
    (unsigned int *type_mask,
    const char *argv[],
    const char *envp[]);

OPENVPN_PLUGIN_DEF int OPENVPN_PLUGIN_FUNC(openvpn_plugin_func_v1)
    (openvpn_plugin_handle_t handle, const int type, const char *argv[], const char *envp[]);

#ifdef __cplusplus
}
#endif

#endif /* OPENVPN_PLUGIN_H_ */
