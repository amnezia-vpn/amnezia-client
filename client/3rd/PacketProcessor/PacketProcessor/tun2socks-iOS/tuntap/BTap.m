/**
 * @file BTap.c
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 *
 * @section LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdio.h>

#ifdef BADVPN_USE_WINAPI
#include <windows.h>
#include <winioctl.h>
#include <objbase.h>
#include <wtypes.h>
#include "wintap-common.h"
#include <tuntap/tapwin32-funcs.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
//    #include <net/if_arp.h>
#ifdef BADVPN_LINUX
#include <linux/if_tun.h>
#endif
#ifdef BADVPN_FREEBSD
#ifdef __APPLE__
#include <ctype.h>
//            #include <net/if_utun.h>
//            #include <sys/sys_domain.h>
//            #include <sys/kern_control.h>
#include <netinet/ip.h>
#include <sys/uio.h>
#else
#include <net/if_tun.h>
#include <net/if_tap.h>
#endif
#endif
#endif

#include "base/BLog.h"

#include "tuntap/BTap.h"

#include "generated/blog_channel_BTap.h"
#include "TunnelInterface.h"

static void report_error (BTap *o);
static void output_handler_recv (BTap *o, uint8_t *data);

#ifdef BADVPN_USE_WINAPI

static void recv_olap_handler (BTap *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->output_packet)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set no output packet
    o->output_packet = NULL;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "read operation failed");
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->frame_mtu)
    
    // done
    PacketRecvInterface_Done(&o->output, bytes);
}

#else

#ifdef __APPLE__

static inline int header_modify_read_write_return (int len)
{
    if (len > 0) {
        return len > sizeof (u_int32_t) ? len - sizeof (u_int32_t) : 0;
    } else {
        return len;
    }
}

static int write_tun_header (int fd, void *buf, size_t len)
{
    u_int32_t type;
    struct iovec iv[2];
    struct ip *iph;
    
    iph = (struct ip *) buf;
    
    if (iph->ip_v == 6) {
        type = htonl(AF_INET6);
    } else {
        type = htonl(AF_INET);
    }
    
    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = len;
    
    return header_modify_read_write_return(writev(fd, iv, 2));
}

static int read_tun_header (int fd, void *buf, size_t len)
{
    u_int32_t type;
    struct iovec iv[2];
    
    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = len;
    
    return header_modify_read_write_return(readv(fd, iv, 2));
}

#endif

static void fd_handler (BTap *o, int events)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    if (events&(BREACTOR_ERROR|BREACTOR_HUP)) {
        BLog(BLOG_WARNING, "device fd reports error?");
    }
    
    if (events&BREACTOR_READ) do {
        ASSERT(o->output_packet)
        
        // try reading into the buffer
#ifdef __APPLE__
        //        int bytes = read_tun_header(o->fd, o->output_packet, o->frame_mtu);
        uint8_t data[2];
        int bytes = read(o->fd, data, 2);
        if (bytes != 2) {
            // Treat zero return value the same as EAGAIN.
            // See: https://bugzilla.kernel.org/show_bug.cgi?id=96381
//            if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
//                // retry later
//                break;
//            }
//            // report fatal error
//            report_error(o);
//            return;
            break;
        }
        int data_len = data[0] * 256 + data[1];
        
        bytes = read(o->fd, o->output_packet, data_len);
#else
        int bytes = read(o->fd, o->output_packet, o->frame_mtu);
#endif
        if (bytes != data_len) {
            // report fatal error
            report_error(o);
            return;
        }
        if (bytes <= 0) {
            // Treat zero return value the same as EAGAIN.
            // See: https://bugzilla.kernel.org/show_bug.cgi?id=96381
            if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
                // retry later
                break;
            }
            // report fatal error
            report_error(o);
            return;
        }
        
#if TCP_DATA_LOG_ENABLE
        BLog(BLOG_DEBUG, "tun2socks receive from tunnel data<len: %d>", bytes);
#endif
        
        ASSERT_FORCE(bytes <= o->frame_mtu)
        
        // set no output packet
        o->output_packet = NULL;
        
        // update events
        o->poll_events &= ~BREACTOR_READ;
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
        
        // inform receiver we finished the packet
        PacketRecvInterface_Done(&o->output, bytes);
    } while (0);
}

#endif

void report_error (BTap *o)
{
    DEBUGERROR(&o->d_err, o->handler_error(o->handler_error_user));
}

void output_handler_recv (BTap *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(data)
    ASSERT(!o->output_packet)
    
#ifdef BADVPN_USE_WINAPI
    
    memset(&o->recv_olap.olap, 0, sizeof(o->recv_olap.olap));
    
    // read
    BOOL res = ReadFile(o->device, data, o->frame_mtu, NULL, &o->recv_olap.olap);
    if (res == FALSE && GetLastError() != ERROR_IO_PENDING) {
        BLog(BLOG_ERROR, "ReadFile failed (%u)", GetLastError());
        report_error(o);
        return;
    }
    
    o->output_packet = data;
    
#else
    
    // attempt read
//#ifdef __APPLE__
//    //    int bytes = read_tun_header(o->fd, data, o->frame_mtu);
//    char msg[2];
//    int bytes = read(o->fd, msg, 2);
//    NSLog(@"fd2 bytes: %d", bytes);
//    
//    if (bytes <= 0) {
//        if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
//            // See note about zero return in fd_handler.
//            // retry later in fd_handler
//            // remember packet
//            o->output_packet = data;
//            // update events
//            o->poll_events |= BREACTOR_READ;
//            BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
//            return;
//        }
//        // report fatal error
//        report_error(o);
//        return;
//    }
//    int len = msg[0] * 256 + msg[1];
//    NSLog(@"fd2 len: %x, %x, len: %d", data[0], data[1], len);
//    bytes = read(o->fd, data, len);
//#else
//    int bytes = read(o->fd, data, o->frame_mtu);
//#endif
//    if (bytes <= 0) {
//        if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
//            // See note about zero return in fd_handler.
//            // retry later in fd_handler
//            // remember packet
//            o->output_packet = data;
//            // update events
//            o->poll_events |= BREACTOR_READ;
//            BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
//            return;
//        }
//        // report fatal error
//        report_error(o);
//        return;
//    }
//    
//    NSLog(@"tun2 receive: %@", [[NSData alloc] initWithBytes:data length:bytes]);
//    
//    ASSERT_FORCE(bytes <= o->frame_mtu)
//    
//    PacketRecvInterface_Done(&o->output, bytes);
    o->output_packet = data;
    // update events
    o->poll_events |= BREACTOR_READ;
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
    
#endif
}

int BTap_Init (BTap *o, BReactor *reactor, int fd, int mtu, BTap_handler_error handler_error, void *handler_error_user, int tun)
{
    ASSERT(tun == 0 || tun == 1)
    
    struct BTap_init_data init_data;
    init_data.dev_type = tun ? BTAP_DEV_TUN : BTAP_DEV_TAP;
    init_data.init_type = BTAP_INIT_FD;
    init_data.init.fd.fd = fd;
    init_data.init.fd.mtu = mtu;
    
    return BTap_Init2(o, reactor, init_data, handler_error, handler_error_user);
}

int BTap_Init2 (BTap *o, BReactor *reactor, struct BTap_init_data init_data, BTap_handler_error handler_error, void *handler_error_user)
{
    ASSERT(init_data.dev_type == BTAP_DEV_TUN || init_data.dev_type == BTAP_DEV_TAP)
    
    // init arguments
    o->reactor = reactor;
    o->handler_error = handler_error;
    o->handler_error_user = handler_error_user;
    
#if defined(BADVPN_LINUX) || defined(BADVPN_FREEBSD)
    
    o->close_fd = (init_data.init_type != BTAP_INIT_FD);
    
    switch (init_data.init_type) {
        case BTAP_INIT_FD: {
            ASSERT(init_data.init.fd.fd >= 0)
            ASSERT(init_data.init.fd.mtu >= 0)
            ASSERT(init_data.dev_type != BTAP_DEV_TAP || init_data.init.fd.mtu >= BTAP_ETHERNET_HEADER_LENGTH)
            
            o->fd = init_data.init.fd.fd;
            o->frame_mtu = init_data.init.fd.mtu;
        } break;
            
        case BTAP_INIT_STRING:
            break;
            
        default: ASSERT(0);
    }
    
    // set non-blocking
    if (fcntl(o->fd, F_SETFL, O_NONBLOCK) < 0) {
        BLog(BLOG_ERROR, "cannot set non-blocking");
        goto fail1;
    }
    
    // init file descriptor object
    BFileDescriptor_Init(&o->bfd, o->fd, (BFileDescriptor_handler)fd_handler, o);
    if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    o->poll_events = 0;
    
    goto success;
    
fail1:
    if (o->close_fd) {
        ASSERT_FORCE(close(o->fd) == 0)
    }
fail0:
    return 0;
    
#endif
    
success:
    // init output
    PacketRecvInterface_Init(&o->output, o->frame_mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, BReactor_PendingGroup(o->reactor));
    
    // set no output packet
    o->output_packet = NULL;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
}

void BTap_Free (BTap *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // free output
    PacketRecvInterface_Free(&o->output);
    
#ifdef BADVPN_USE_WINAPI
    
    // cancel I/O
    ASSERT_FORCE(CancelIo(o->device))
    
    // wait receiving to finish
    if (o->output_packet) {
        BLog(BLOG_DEBUG, "waiting for receiving to finish");
        BReactorIOCPOverlapped_Wait(&o->recv_olap, NULL, NULL);
    }
    
    // free recv olap
    BReactorIOCPOverlapped_Free(&o->recv_olap);
    
    // free send olap
    BReactorIOCPOverlapped_Free(&o->send_olap);
    
    // close device
    ASSERT_FORCE(CloseHandle(o->device))
    
#else
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    
    if (o->close_fd) {
        // close file descriptor
        ASSERT_FORCE(close(o->fd) == 0)
    }
    
#endif
}

int BTap_GetMTU (BTap *o)
{
    DebugObject_Access(&o->d_obj);
    
    return o->frame_mtu;
}

void BTap_Send (BTap *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->frame_mtu)
    
#ifdef BADVPN_USE_WINAPI
    
    // ignore frames without an Ethernet header, or we get errors in WriteFile
    if (data_len < 14) {
        return;
    }
    
    memset(&o->send_olap.olap, 0, sizeof(o->send_olap.olap));
    
    // write
    BOOL res = WriteFile(o->device, data, data_len, NULL, &o->send_olap.olap);
    if (res == FALSE && GetLastError() != ERROR_IO_PENDING) {
        BLog(BLOG_ERROR, "WriteFile failed (%u)", GetLastError());
        return;
    }
    
    // wait
    int succeeded;
    DWORD bytes;
    BReactorIOCPOverlapped_Wait(&o->send_olap, &succeeded, &bytes);
    
    if (!succeeded) {
        BLog(BLOG_ERROR, "write operation failed");
    } else {
        ASSERT(bytes >= 0)
        ASSERT(bytes <= data_len)
        
        if (bytes < data_len) {
            BLog(BLOG_ERROR, "write operation didn't write everything");
        }
    }
    
#else
    
#ifdef __APPLE__
    //    int bytes = write_tun_header(o->fd, data, data_len);
    NSData *outdata = [[NSData alloc] initWithBytes:data length:data_len];
#if TCP_DATA_LOG_ENABLE
    BLog(BLOG_DEBUG, "tun2socks send to tunnel data<len: %d>", data_len);
#endif
    [TunnelInterface writePacket:outdata];
    return;
    uint8_t msg[o->frame_mtu+2];
    msg[0] = data_len / 256;
    msg[1] = data_len % 256;
    memcpy(msg + 2, data, data_len);

    int bytes = write(o->fd, msg, data_len + 2);
#else
    int bytes = write(o->fd, data, data_len);
#endif
    if (bytes < 0) {
        // malformed packets will cause errors, ignore them and act like
        // the packet was accepeted
    } else {
        if (bytes != data_len + 2) {
            BLog(BLOG_WARNING, "written %d expected %d", bytes, data_len + 2);
        }
    }
    
#endif
}

PacketRecvInterface * BTap_GetOutput (BTap *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}
