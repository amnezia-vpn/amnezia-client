/*
 * Copyright (c) 2008-2013 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1983, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__unused static const char copyright[] =
"@(#) Copyright (c) 1983, 1989, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <ifaddrs.h>

struct keytab {
	char	*kt_cp;
	int	kt_i;
} keywords[] = {
        #define	K_ADD	1
            {"add", K_ADD},
        #define	K_BLACKHOLE	2
            {"blackhole", K_BLACKHOLE},
        #define	K_CHANGE	3
            {"change", K_CHANGE},
        #define	K_CLONING	4
            {"cloning", K_CLONING},
        #define	K_DELETE	5
            {"delete", K_DELETE},
        #define	K_DST	6
            {"dst", K_DST},
        #define	K_EXPIRE	7
            {"expire", K_EXPIRE},
        #define	K_FLUSH	8
            {"flush", K_FLUSH},
        #define	K_GATEWAY	9
            {"gateway", K_GATEWAY},
        #define	K_GENMASK	10
            {"genmask", K_GENMASK},
        #define	K_GET	11
            {"get", K_GET},
        #define	K_HOST	12
            {"host", K_HOST},
        #define	K_HOPCOUNT	13
            {"hopcount", K_HOPCOUNT},
        #define	K_IFACE	14
            {"iface", K_IFACE},
        #define	K_INTERFACE	15
            {"interface", K_INTERFACE},
        #define	K_IFA	16
            {"ifa", K_IFA},
        #define	K_IFP	17
            {"ifp", K_IFP},
        #define	K_INET	18
            {"inet", K_INET},
        #define	K_INET6	19
            {"inet6", K_INET6},
        #define	K_ISO	20
            {"iso", K_ISO},
        #define	K_LINK	21
            {"link", K_LINK},
        #define	K_LLINFO	22
            {"llinfo", K_LLINFO},
        #define	K_LOCK	23
            {"lock", K_LOCK},
        #define	K_LOCKREST	24
            {"lockrest", K_LOCKREST},
        #define	K_MASK	25
            {"mask", K_MASK},
        #define	K_MONITOR	26
            {"monitor", K_MONITOR},
        #define	K_MTU	27
            {"mtu", K_MTU},
        #define	K_NET	28
            {"net", K_NET},
        #define	K_NETMASK	29
            {"netmask", K_NETMASK},
        #define	K_NOSTATIC	30
            {"nostatic", K_NOSTATIC},
        #define	K_OSI	31
            {"osi", K_OSI},
        #define	K_PREFIXLEN	32
            {"prefixlen", K_PREFIXLEN},
        #define	K_PROTO1	33
            {"proto1", K_PROTO1},
        #define	K_PROTO2	34
            {"proto2", K_PROTO2},
        #define	K_RECVPIPE	35
            {"recvpipe", K_RECVPIPE},
        #define	K_REJECT	36
            {"reject", K_REJECT},
        #define	K_RTT	37
            {"rtt", K_RTT},
        #define	K_RTTVAR	38
            {"rttvar", K_RTTVAR},
        #define	K_SA	39
            {"sa", K_SA},
        #define	K_SENDPIPE	40
            {"sendpipe", K_SENDPIPE},
        #define	K_SSTHRESH	41
            {"ssthresh", K_SSTHRESH},
        #define	K_STATIC	42
            {"static", K_STATIC},
        #define	K_X25	43
            {"x25", K_X25},
        #define	K_XNS	44
            {"xns", K_XNS},
        #define	K_XRESOLVE	45
            {"xresolve", K_XRESOLVE},
        #define	K_IFSCOPE	46
            {"ifscope", K_IFSCOPE},
	{0, 0}
};

union	sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in sin;
#ifdef INET6
	struct	sockaddr_in6 sin6;
#endif
	struct	sockaddr_dl sdl;
	struct	sockaddr_storage ss; /* added to avoid memory overrun */
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

typedef union sockunion *sup;
int	pid, rtm_addrs, uid;
int	s;
int	forcehost, forcenet, doflush, nflag, af, qflag, tflag, keyword();
int	iflag, verbose, aflen = sizeof (struct sockaddr_in);
int	locking, lockrest, debugonly;
struct	rt_metrics rt_metrics;
u_long  rtm_inits;
unsigned int ifscope;

static const char *route_strerror(int);
const char	*routename(), *netname();
void	flushroutes(), newroute(), monitor(), sockaddr(), sodump(), bprintf();
void	print_getmsg(), print_rtmsg(), pmsg_common(), pmsg_addrs(), mask_addr();
int	getaddr(), rtmsg(), x25_makemask();
int	prefixlen();
extern	char *iso_ntoa();

static void
inet_makenetandmask(in_addr_t net, struct sockaddr_in *sin,
    struct sockaddr_in *sin_mask, in_addr_t bits);

void usage __P((const char *));

void
usage(cp)
	const char *cp;
{
	if (cp)
		warnx("bad keyword: %s", cp);
	(void) fprintf(stderr,
	    "usage: route [-dnqtv] command [[modifiers] args]\n");
    //exit(EX_USAGE);
	/* NOTREACHED */
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int
mainRouteIface(argc, argv)
    int argc;
    char **argv;
{
    // reset static vars
    pid = rtm_addrs = uid = 0;
    s = 0;
    forcehost = forcenet = doflush = nflag = af = qflag = tflag = 0;
    iflag = verbose = 0;
    locking = lockrest = debugonly = 0;
    rtm_inits = 0;
    ifscope = 0;
    //

    int ch;

    if (argc < 2)
        usage((char *)NULL);

    while ((ch = getopt(argc, argv, "nqdtv")) != -1)
        switch(ch) {
        case 'n':
            nflag = 1;
            break;
        case 'q':
            qflag = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 't':
            tflag = 1;
            break;
        case 'd':
            debugonly = 1;
            break;
        case '?':
        default:
            usage((char *)NULL);
        }
    argc -= optind;
    argv += optind;

    pid = getpid();
    uid = geteuid();
    if (tflag)
        s = open(_PATH_DEVNULL, O_WRONLY, 0);
    else
        s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        err(EX_OSERR, "socket");
    setuid(uid);
    if (*argv)
        switch (keyword(*argv)) {
        case K_GET:
            uid = 0;
            /* FALLTHROUGH */

        case K_CHANGE:
        case K_ADD:
        case K_DELETE:
            newroute(argc, argv);
            break;
            /* NOTREACHED */

        case K_MONITOR:
            monitor();
            break;
            /* NOTREACHED */

        case K_FLUSH:
            flushroutes(argc, argv);
            break;
            /* NOTREACHED */
        }
    close(s);
    fflush(stdout);
    return 0;
    //usage(*argv);
    /* NOTREACHED */
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
void
flushroutes(argc, argv)
	int argc;
	char *argv[];
{
	size_t needed;
	int mib[6], rlen, seqno;
	char *buf, *next, *lim;
	register struct rt_msghdr *rtm;

	if (uid) {
		errx(EX_NOPERM, "must be root to alter routing table");
	}
	shutdown(s, 0); /* Don't want to read back our messages */
	if (argc > 1) {
		argv++;
		if (argc == 2 && **argv == '-')
		    switch (keyword(*argv + 1)) {
			case K_INET:
				af = AF_INET;
				break;
#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				break;
#endif
			case K_LINK:
				af = AF_LINK;
				break;
			default:
				goto bad;
		} else
bad:			usage(*argv);
	}
	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(EX_OSERR, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		errx(EX_OSERR, "malloc failed");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(EX_OSERR, "route-sysctl-get");
	lim = buf + needed;
	if (verbose)
		(void) printf("Examining routing table from sysctl\n");
	seqno = 0;		/* ??? */
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		if (verbose)
			print_rtmsg(rtm, rtm->rtm_msglen);
		if ((rtm->rtm_flags & RTF_GATEWAY) == 0)
			continue;
		if (af) {
			struct sockaddr *sa = (struct sockaddr *)(rtm + 1);

			if (sa->sa_family != af)
				continue;
		}
		if (debugonly)
			continue;
		rtm->rtm_type = RTM_DELETE;
		rtm->rtm_seq = seqno;
		rlen = write(s, next, rtm->rtm_msglen);
		if (rlen < (int)rtm->rtm_msglen) {
			warn("write to routing socket");
			(void) printf("got only %d for rlen\n", rlen);
			break;
		}
		seqno++;
		if (qflag)
			continue;
		if (verbose)
			print_rtmsg(rtm, rlen);
		else {
			struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
			(void) printf("%-20.20s ", rtm->rtm_flags & RTF_HOST ?
			    routename(sa) : netname(sa));
			sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
			(void) printf("%-20.20s ", routename(sa));
			(void) printf("done\n");
		}
	}
}

const char *
routename(sa)
	struct sockaddr *sa;
{
	register char *cp;
	static char line[MAXHOSTNAMELEN + 1];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.'))) {
			domain[MAXHOSTNAMELEN] = '\0';
			(void) memmove(domain, cp + 1, strlen(cp + 1) + 1);
		} else
			domain[0] = 0;
	}

	if (sa->sa_len == 0)
		strlcpy(line, "default", sizeof(line));
	else switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		cp = 0;
		if (in.s_addr == INADDR_ANY || sa->sa_len < 4)
			cp = "default";
		if (cp == 0 && !nflag) {
			hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
				AF_INET);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		if (cp) {
			strlcpy(line, cp, sizeof(line));
		} else {
			/* XXX - why not inet_ntoa()? */
#define C(x)	(unsigned)((x) & 0xff)
			in.s_addr = ntohl(in.s_addr);
			(void) snprintf(line, sizeof(line), "%u.%u.%u.%u", C(in.s_addr >> 24),
			   C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
		}
		break;
	    }

#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 sin6; /* use static var for safety */
		int niflags = 0;
#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#endif

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_NODELOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strlcpy(line, "invalid", sizeof(line));

		return(line);
	}
#endif

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

	default:
	    {	u_short *s = (u_short *)sa;
		u_short *slim = s + ((sa->sa_len + 1) >> 1);
		char *cp = line + snprintf(line, sizeof(line), "(%d)", sa->sa_family);
		char *cpe = line + sizeof(line);

		while (++s < slim && cp < cpe) /* start with sa->sa_data */
			cp += snprintf(cp, cpe - cp, " %x", *s);
		break;
	    }
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net, not a host.
 */
const char *
netname(sa)
	struct sockaddr *sa;
{
	char *cp = NULL;
	static char line[MAXHOSTNAMELEN + 1];
	struct netent *np = NULL;
	register in_addr_t i;

	switch (sa->sa_family) {

		case AF_INET:
		   {   struct in_addr in;
		       in = ((struct sockaddr_in *)sa)->sin_addr;

		       i = in.s_addr = ntohl(in.s_addr);
		       if (in.s_addr == 0)
			       cp = "default";
		       else if (!nflag) {
			       np = getnetbyaddr(i, AF_INET);
			       if (np != NULL)
				       cp = np->n_name;
		       }
#define C(x)    (unsigned)((x) & 0xff)
		       if (cp != NULL)
			       strlcpy(line, cp, sizeof(line));
		       else if ((in.s_addr & 0xffffff) == 0)
			       (void) sprintf(line, "%u", C(in.s_addr >> 24));
		       else if ((in.s_addr & 0xffff) == 0)
			       (void) sprintf(line, "%u.%u", C(in.s_addr >> 24),
					       C(in.s_addr >> 16));
		       else if ((in.s_addr & 0xff) == 0)
			       (void) sprintf(line, "%u.%u.%u", C(in.s_addr >> 24),
					       C(in.s_addr >> 16), C(in.s_addr >> 8));
		       else
			       (void) sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
					       C(in.s_addr >> 16), C(in.s_addr >> 8),
					       C(in.s_addr));
#undef C
		       break;
		   }
#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 sin6; /* use static var for safety */
		int niflags = 0;
#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#endif

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_NODELOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strlcpy(line, "invalid", sizeof(line));

		return(line);
	}
#endif

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));


	default:
	    {	u_short *s = (u_short *)sa->sa_data;
		u_short *slim = s + ((sa->sa_len + 1)>>1);
		char *cp = line + snprintf(line, sizeof(line), "af %d:", sa->sa_family);
		char *cpe = line + sizeof(line);

		while (s < slim && cp < cpe)
			cp += snprintf(cp, cpe - cp, " %x", *s++);
		break;
	    }
	}
	return (line);
}

static const char *
route_strerror(int error)
{

	switch (error) {
	case ESRCH:
		return "not in table";
	case EBUSY:
		return "entry in use";
	case ENOBUFS:
		return "routing table overflow";
	default:
		return (strerror(error));
	}
}

void
set_metric(value, key)
	char *value;
	int key;
{
	int flag = 0;
	u_int noval, *valp = &noval;

	switch (key) {
#define caseof(x, y, z)	case x: valp = (u_int *)&rt_metrics.z; flag = y; break
	caseof(K_MTU, RTV_MTU, rmx_mtu);
	caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
	caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
	caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
	caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
	caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
	caseof(K_RTT, RTV_RTT, rmx_rtt);
	caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
	}
	rtm_inits |= flag;
	if (lockrest || locking)
		rt_metrics.rmx_locks |= flag;
	if (locking)
		locking = 0;
	*valp = atoi(value);
}

void
newroute(argc, argv)
	int argc;
	register char **argv;
{
	char *cmd, *dest = "", *gateway = "";
	int ishost = 0, ret, attempts, oerrno, flags = RTF_STATIC;
	int key;
	struct hostent *hp = 0;

	if (uid) {
		errx(EX_NOPERM, "must be root to alter routing table");
	}
	cmd = argv[0];
	if (*cmd != 'g')
		shutdown(s, 0); /* Don't want to read back our messages */
	while (--argc > 0) {
		if (**(++argv)== '-') {
			switch (key = keyword(1 + *argv)) {
			case K_LINK:
				af = AF_LINK;
				aflen = sizeof(struct sockaddr_dl);
				break;
			case K_INET:
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
				break;
#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				aflen = sizeof(struct sockaddr_in6);
				break;
#endif
			case K_SA:
				af = PF_ROUTE;
				aflen = sizeof(union sockunion);
				break;
			case K_IFACE:
			case K_INTERFACE:
				iflag++;
				break;
			case K_NOSTATIC:
				flags &= ~RTF_STATIC;
				break;
			case K_LLINFO:
				flags |= RTF_LLINFO;
				break;
			case K_LOCK:
				locking = 1;
				break;
			case K_LOCKREST:
				lockrest = 1;
				break;
			case K_HOST:
				forcehost++;
				break;
			case K_REJECT:
				flags |= RTF_REJECT;
				break;
			case K_BLACKHOLE:
				flags |= RTF_BLACKHOLE;
				break;
			case K_PROTO1:
				flags |= RTF_PROTO1;
				break;
			case K_PROTO2:
				flags |= RTF_PROTO2;
				break;
			case K_CLONING:
				flags |= RTF_CLONING;
				break;
			case K_XRESOLVE:
				flags |= RTF_XRESOLVE;
				break;
			case K_STATIC:
				flags |= RTF_STATIC;
				break;
			case K_IFA:
				if (!--argc)
					usage((char *)NULL);
				(void) getaddr(RTA_IFA, *++argv, 0);
				break;
			case K_IFP:
				if (!--argc)
					usage((char *)NULL);
				(void) getaddr(RTA_IFP, *++argv, 0);
				break;
			case K_GENMASK:
				if (!--argc)
					usage((char *)NULL);
				(void) getaddr(RTA_GENMASK, *++argv, 0);
				break;
			case K_GATEWAY:
				if (!--argc)
					usage((char *)NULL);
				(void) getaddr(RTA_GATEWAY, *++argv, 0);
				break;
			case K_DST:
				if (!--argc)
					usage((char *)NULL);
				ishost = getaddr(RTA_DST, *++argv, &hp);
				dest = *argv;
				break;
			case K_NETMASK:
				if (!--argc)
					usage((char *)NULL);
				(void) getaddr(RTA_NETMASK, *++argv, 0);
				/* FALLTHROUGH */
			case K_NET:
				forcenet++;
				break;
			case K_PREFIXLEN:
				if (!--argc)
					usage((char *)NULL);
				if (prefixlen(*++argv) == -1) {
					forcenet = 0;
					ishost = 1;
				} else {
					forcenet = 1;
					ishost = 0;
				}
				break;
			case K_MTU:
			case K_HOPCOUNT:
			case K_EXPIRE:
			case K_RECVPIPE:
			case K_SENDPIPE:
			case K_SSTHRESH:
			case K_RTT:
			case K_RTTVAR:
				if (!--argc)
					usage((char *)NULL);
				set_metric(*++argv, key);
				break;
			case K_IFSCOPE:
				if (!--argc)
					usage((char *)NULL);
				if ((ifscope = if_nametoindex(*++argv)) != 0)
					flags |= RTF_IFSCOPE;
				else
					errx(1, "bad interface name");
				break;
			default:
				usage(1+*argv);
			}
		} else {
			if ((rtm_addrs & RTA_DST) == 0) {
				dest = *argv;
				ishost = getaddr(RTA_DST, *argv, &hp);
			} else if ((rtm_addrs & RTA_GATEWAY) == 0) {
				gateway = *argv;
				(void) getaddr(RTA_GATEWAY, *argv, &hp);
			} else {
				(void) getaddr(RTA_NETMASK, *argv, 0);
			}
		}
	}
	if (forcehost) {
		ishost = 1;
#ifdef INET6
		if (af == AF_INET6) {
			rtm_addrs &= ~RTA_NETMASK;
			memset((void *)&so_mask, 0, sizeof(so_mask));
		}
#endif 
	}
	if (forcenet)
		ishost = 0;
	flags |= RTF_UP;
	if (ishost)
		flags |= RTF_HOST;
	if (iflag == 0)
		flags |= RTF_GATEWAY;
	if (so_mask.sin.sin_family == AF_INET) {
		// make sure the mask is contiguous
		long i;
		for (i = 0; i < 32; i++)
			if (((so_mask.sin.sin_addr.s_addr) & ntohl((1 << i))) != 0)
				break;
		for (; i < 32; i++)
			if (((so_mask.sin.sin_addr.s_addr) & ntohl((1 << i))) == 0)
				errx(EX_NOHOST, "invalid mask: %s", inet_ntoa(so_mask.sin.sin_addr));
	}
	for (attempts = 1; ; attempts++) {
		errno = 0;
		if ((ret = rtmsg(*cmd, flags)) == 0)
			break;
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (af == AF_INET && *gateway && hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], &so_gate.sin.sin_addr,
			    MIN(hp->h_length, sizeof(so_gate.sin.sin_addr)));
		} else
			break;
	}
	if (*cmd == 'g')
        return;
	oerrno = errno;
//	(void) printf("%s %s %s", cmd, ishost? "host" : "net", dest);
//	if (*gateway) {
//		(void) printf(": gateway %s", gateway);
//		if (attempts > 1 && ret == 0 && af == AF_INET)
//		    (void) printf(" (%s)", inet_ntoa(so_gate.sin.sin_addr));
//	}
//	if (ret == 0)
//		(void) printf("\n");
//	else {
//		(void)printf(": %s\n", route_strerror(oerrno));
//	}
}

static void
inet_makenetandmask(in_addr_t net, struct sockaddr_in *sin,
    struct sockaddr_in *sin_mask, in_addr_t bits)
{
	in_addr_t mask = 0;
	
	rtm_addrs |= RTA_NETMASK;
	/*
	 * MSB of net should be meaningful. 0/0 is exception.
	 */
	if (net > 0)
		while ((net & 0xff000000) == 0)
			net <<= 8;

	/*
	 * If no /xx was specified we must calculate the
	 * CIDR address.
	 */
	if ((bits == 0) && (net != 0)) {
		u_long i, j;

		for(i = 0, j = 0xff; i < 4; i++)  {
			if (net & j) {
				break;
			}
			j <<= 8;
		}
		/* i holds the first non zero bit */
		bits = 32 - (i*8);	
	}
	if (bits != 0)
		mask = 0xffffffff << (32 - bits);

	sin->sin_addr.s_addr = htonl(net);
	sin_mask->sin_addr.s_addr = htonl(mask);
	sin_mask->sin_len = sizeof(struct sockaddr_in);
	sin_mask->sin_family = AF_INET;
}

#ifdef INET6
/*
 * XXX the function may need more improvement...
 */
static int
inet6_makenetandmask(struct sockaddr_in6 *sin6, const char *plen)
{
	struct in6_addr in6;

	if (plen == NULL) {
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
		    sin6->sin6_scope_id == 0) {
			plen = "0";
		} else if ((sin6->sin6_addr.s6_addr[0] & 0xe0) == 0x20) {
			/* aggregatable global unicast - RFC2374 */
			memset(&in6, 0, sizeof(in6));
			if (!memcmp(&sin6->sin6_addr.s6_addr[8],
				    &in6.s6_addr[8], 8))
				plen = "64";
		}
	}

	if (plen == NULL || strcmp(plen, "128") == 0)
		return (1);
	rtm_addrs |= RTA_NETMASK;
	prefixlen(plen);
	return (0);
}
#endif

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
int
getaddr(which, s, hpp)
	int which;
	char *s;
	struct hostent **hpp;
{
	register sup su = NULL;
	struct hostent *hp;
	struct netent *np;
	in_addr_t val;
	char *q;
	int afamily;  /* local copy of af so we can change it */

	if (af == 0) {
		af = AF_INET;
		aflen = sizeof(struct sockaddr_in);
	}
	afamily = af;
	rtm_addrs |= which;
	switch (which) {
	case RTA_DST:
		su = &so_dst;
		break;
	case RTA_GATEWAY:
		su = &so_gate;
		if (iflag) {
			struct ifaddrs *ifap, *ifa;
			struct sockaddr_dl *sdl = NULL;

			if (getifaddrs(&ifap))
				err(1, "getifaddrs");

			for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
				if (ifa->ifa_addr->sa_family != AF_LINK)
					continue;

				if (strcmp(s, ifa->ifa_name))
					continue;

				sdl = (struct sockaddr_dl *)ifa->ifa_addr;
			}
			/* If we found it, then use it */
			if (sdl) {
				/*
				 * Copy is safe since we have a
				 * sockaddr_storage member in sockunion{}.
				 * Note that we need to copy before calling
				 * freeifaddrs().
				 */
				memcpy(&su->sdl, sdl, sdl->sdl_len);
			}
			freeifaddrs(ifap);
			if (sdl)
				return(1);
		}
		break;
	case RTA_NETMASK:
		su = &so_mask;
		break;
	case RTA_GENMASK:
		su = &so_genmask;
		break;
	case RTA_IFP:
		su = &so_ifp;
		afamily = AF_LINK;
		break;
	case RTA_IFA:
		su = &so_ifa;
		break;
	default:
		usage("internal error");
		/*NOTREACHED*/
	}
	su->sa.sa_len = aflen;
	su->sa.sa_family = afamily; /* cases that don't want it have left already */
	if (strcmp(s, "default") == 0) {
		/*
		 * Default is net 0.0.0.0/0 
		 */
		switch (which) {
		case RTA_DST:
			forcenet++;
			/* bzero(su, sizeof(*su)); *//* for readability */
			(void) getaddr(RTA_NETMASK, s, 0);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			/* bzero(su, sizeof(*su)); *//* for readability */
			su->sa.sa_len = 0;
			break;
		}
		return (0);
	}
	switch (afamily) {
#ifdef INET6
	case AF_INET6:
	{
		struct addrinfo hints, *res;
		int ecode;

		q = NULL;
		if (which == RTA_DST && (q = strchr(s, '/')) != NULL)
			*q = '\0';
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = afamily;	/*AF_INET6*/
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
		ecode = getaddrinfo(s, NULL, &hints, &res);
		if (ecode != 0 || res->ai_family != AF_INET6 ||
		    res->ai_addrlen != sizeof(su->sin6)) {
			(void) fprintf(stderr, "%s: %s\n", s,
			    gai_strerror(ecode));
			exit(1);
		}
		memcpy(&su->sin6, res->ai_addr, sizeof(su->sin6));
#ifdef __KAME__
		if ((IN6_IS_ADDR_LINKLOCAL(&su->sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_NODELOCAL(&su->sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&su->sin6.sin6_addr)) &&
		    su->sin6.sin6_scope_id) {
			*(u_int16_t *)&su->sin6.sin6_addr.s6_addr[2] =
				htons(su->sin6.sin6_scope_id);
			su->sin6.sin6_scope_id = 0;
		}
#endif
		freeaddrinfo(res);
		if (hints.ai_flags == AI_NUMERICHOST) {
			if (q != NULL)
				*q++ = '/';
			if (which == RTA_DST)
				return (inet6_makenetandmask(&su->sin6, q));
			return (0);
		} else {
			return (1);
		}
	}
#endif /* INET6 */

	case AF_LINK:
		link_addr(s, &su->sdl);
		return (1);


	case PF_ROUTE:
		su->sa.sa_len = sizeof(*su);
		sockaddr(s, &su->sa);
		return (1);

	case AF_INET:
	default:
		break;
	}

	if (hpp == NULL)
		hpp = &hp;
	*hpp = NULL;

	q = strchr(s,'/');
	if (q && which == RTA_DST) {
		*q = '\0';
		if ((val = inet_network(s)) != INADDR_NONE) {
			inet_makenetandmask(
				val, &su->sin, (struct sockaddr_in *)&so_mask,
				strtoul(q+1, 0, 0));
			return (0);
		}
		*q = '/';
	}
	if ((which != RTA_DST || forcenet == 0) &&
	    inet_aton(s, &su->sin.sin_addr)) {
		val = su->sin.sin_addr.s_addr;
		if (which != RTA_DST || forcehost ||
		    inet_lnaof(su->sin.sin_addr) != INADDR_ANY)
			return (1);
		else {
			val = ntohl(val);
			goto netdone;
		}
	}
	if (which == RTA_DST && forcehost == 0 &&
	    ((val = inet_network(s)) != INADDR_NONE ||
	    ((np = getnetbyname(s)) != NULL && (val = np->n_net) != 0))) {
netdone:
		inet_makenetandmask(val, &su->sin, (struct sockaddr_in *)&so_mask, 0);
		return (0);
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		su->sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (char *)&su->sin.sin_addr, 
		    MIN(hp->h_length, sizeof(su->sin.sin_addr)));
		return (1);
	}
	errx(EX_NOHOST, "bad address: %s", s);
}

int
prefixlen(s)
	char *s;
{
	int len = atoi(s), q, r;
	int max;
	char *p;

	rtm_addrs |= RTA_NETMASK;	
	switch (af) {
#ifdef INET6
	case AF_INET6:
		max = 128;
		p = (char *)&so_mask.sin6.sin6_addr;
		break;
#endif
	case AF_INET:
		max = 32;
		p = (char *)&so_mask.sin.sin_addr;
		break;
	default:
		(void) fprintf(stderr, "prefixlen not supported in this af\n");
        return(1);
		/*NOTREACHED*/
	}

	if (len < 0 || max < len) {
		(void) fprintf(stderr, "%s: bad value\n", s);
        return(1);
	}
	
	q = len >> 3;
	r = len & 7;
	so_mask.sa.sa_family = af;
	so_mask.sa.sa_len = aflen;
	memset((void *)p, 0, max / 8);
	if (q > 0)
		memset((void *)p, 0xff, q);
	if (r > 0)
		*((u_char *)p + q) = (0xff00 >> r) & 0xff;
	if (len == max)
		return -1;
	else
		return len;
}

void
interfaces()
{
	size_t needed;
	int mib[6];
	char *buf, *lim, *next;
	register struct rt_msghdr *rtm;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(EX_OSERR, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		errx(EX_OSERR, "malloc failed");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(EX_OSERR, "actual retrieval of interface table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		print_rtmsg(rtm, rtm->rtm_msglen);
	}
}

void
monitor()
{
	int n;
	char msg[2048];

	verbose = 1;
	if (debugonly) {
		interfaces();
        return;
	}
	for(;;) {
		time_t now;
		n = read(s, msg, 2048);
		now = time(NULL);
		(void) printf("\ngot message of size %d on %s", n, ctime(&now));
		print_rtmsg((struct rt_msghdr *)msg, n);
	}
}

struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

int
rtmsg(cmd, flags)
	int cmd, flags;
{
	static int seq;
	int rlen;
	register char *cp = m_rtmsg.m_space;
	register int l;

#define NEXTADDR(w, u) \
	if (rtm_addrs & (w)) {\
	    l = ROUNDUP(u.sa.sa_len); bcopy((char *)&(u), cp, l); cp += l;\
	}

	errno = 0;
	bzero((char *)&m_rtmsg, sizeof(m_rtmsg));
	if (cmd == 'a')
		cmd = RTM_ADD;
	else if (cmd == 'c')
		cmd = RTM_CHANGE;
	else if (cmd == 'g') {
		cmd = RTM_GET;
		if (so_ifp.sa.sa_family == 0) {
			so_ifp.sa.sa_family = AF_LINK;
			so_ifp.sa.sa_len = sizeof(struct sockaddr_dl);
			rtm_addrs |= RTA_IFP;
		}
	} else
		cmd = RTM_DELETE;
#define rtm m_rtmsg.m_rtm
	rtm.rtm_type = cmd;
	rtm.rtm_flags = flags;
	rtm.rtm_version = RTM_VERSION;
	rtm.rtm_seq = ++seq;
	rtm.rtm_addrs = rtm_addrs;
	rtm.rtm_rmx = rt_metrics;
	rtm.rtm_inits = rtm_inits;
	rtm.rtm_index = ifscope;

	if (rtm_addrs & RTA_NETMASK)
		mask_addr();
	NEXTADDR(RTA_DST, so_dst);
	NEXTADDR(RTA_GATEWAY, so_gate);
	NEXTADDR(RTA_NETMASK, so_mask);
	NEXTADDR(RTA_GENMASK, so_genmask);
	NEXTADDR(RTA_IFP, so_ifp);
	NEXTADDR(RTA_IFA, so_ifa);
	rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;
	if (verbose)
		print_rtmsg(&rtm, l);
	if (debugonly)
		return (0);
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		warnx("writing to routing socket: %s", route_strerror(errno));
		return (-1);
	}
	if (cmd == RTM_GET) {
		do {
			l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
		} while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));
		if (l < 0)
			warn("read from routing socket");
		else
			print_getmsg(&rtm, l);
	}
#undef rtm
	return (0);
}

void
mask_addr()
{
	int olen = so_mask.sa.sa_len;
	register char *cp1 = olen + (char *)&so_mask, *cp2;

	for (so_mask.sa.sa_len = 0; cp1 > (char *)&so_mask; )
		if (*--cp1 != 0) {
			so_mask.sa.sa_len = 1 + cp1 - (char *)&so_mask;
			break;
		}
	if ((rtm_addrs & RTA_DST) == 0)
		return;
	switch (so_dst.sa.sa_family) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
	case AF_APPLETALK:
	case 0:
		return;
	}
	cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
	cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
	while (cp2 > cp1)
		*--cp2 = 0;
	cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
	while (cp1 > so_dst.sa.sa_data)
		*--cp1 &= *--cp2;
}

char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_IFINFO: iface status change",
	"RTM_NEWMADDR: new multicast group membership on iface",
	"RTM_DELMADDR: multicast group membership removed from iface",
	0,
};

char metricnames[] =
"\011pksent\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount"
"\1mtu";
char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010DELCLONE"
"\011CLONING\012XRESOLVE\013LLINFO\014STATIC\015BLACKHOLE\016b016"
"\017PROTO2\020PROTO1\021PRCLONING\022WASCLONED\023PROTO3\024b024"
"\025PINNED\026LOCAL\027BROADCAST\030MULTICAST\031IFSCOPE\032CONDEMNED"
"\033IFREF\034PROXY\035ROUTER";
char ifnetflags[] =
"\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5PTP\6b6\7RUNNING\010NOARP"
"\011PPROMISC\012ALLMULTI\013OACTIVE\014SIMPLEX\015LINK0\016LINK1"
"\017LINK2\020MULTICAST";
char addrnames[] =
"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR\010BRD";

void
print_rtmsg(rtm, msglen)
	register struct rt_msghdr *rtm;
	int msglen;
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
#ifdef RTM_NEWMADDR
	struct ifma_msghdr *ifmam;
#endif

	if (verbose == 0)
		return;
	if (rtm->rtm_version != RTM_VERSION) {
		(void) printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	(void)printf("%s: len %d, ", msgtypes[rtm->rtm_type], rtm->rtm_msglen);
	switch (rtm->rtm_type) {
	case RTM_IFINFO:
		ifm = (struct if_msghdr *)rtm;
		(void) printf("if# %d, flags:", ifm->ifm_index);
		bprintf(stdout, ifm->ifm_flags, ifnetflags);
		pmsg_addrs((char *)(ifm + 1), ifm->ifm_addrs);
		break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
		ifam = (struct ifa_msghdr *)rtm;
		(void) printf("metric %d, flags:", ifam->ifam_metric);
		bprintf(stdout, ifam->ifam_flags, routeflags);
		pmsg_addrs((char *)(ifam + 1), ifam->ifam_addrs);
		break;
#ifdef RTM_NEWMADDR
	case RTM_NEWMADDR:
	case RTM_DELMADDR:
		ifmam = (struct ifma_msghdr *)rtm;
		pmsg_addrs((char *)(ifmam + 1), ifmam->ifmam_addrs);
		break;
#endif
	default:
		(void) printf("pid: %ld, seq %d, errno %d, ",
			(long)rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno);
		if (rtm->rtm_flags & RTF_IFSCOPE)
			(void) printf("ifscope %d, ", rtm->rtm_index);
			if (rtm->rtm_flags & RTF_IFREF)
			(void) printf("ifref, ");
			(void) printf("flags:");
		bprintf(stdout, rtm->rtm_flags, routeflags);
		pmsg_common(rtm);
	}
}

void
print_getmsg(rtm, msglen)
	register struct rt_msghdr *rtm;
	int msglen;
{
	struct sockaddr *dst = NULL, *gate = NULL, *mask = NULL;
	struct sockaddr_dl *ifp = NULL;
	register struct sockaddr *sa;
	register char *cp;
	register int i;

	(void) printf("   route to: %s\n", routename(&so_dst.sa));
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		     rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		      rtm->rtm_msglen, msglen);
	}
	if (rtm->rtm_errno)  {
		errno = rtm->rtm_errno;
		warn("message indicates error %d", errno);
		return;
	}
	cp = ((char *)(rtm + 1));
	if (rtm->rtm_addrs)
		for (i = 1; i; i <<= 1)
			if (i & rtm->rtm_addrs) {
				sa = (struct sockaddr *)cp;
				switch (i) {
				case RTA_DST:
					dst = sa;
					break;
				case RTA_GATEWAY:
					gate = sa;
					break;
				case RTA_NETMASK:
					mask = sa;
					break;
				case RTA_IFP:
					if (sa->sa_family == AF_LINK &&
					   ((struct sockaddr_dl *)sa)->sdl_nlen)
						ifp = (struct sockaddr_dl *)sa;
					break;
				}
				ADVANCE(cp, sa);
			}
	if (dst && mask)
		mask->sa_family = dst->sa_family;	/* XXX */
	if (dst)
		(void)printf("destination: %s\n", routename(dst));
	if (mask) {
		int savenflag = nflag;

		nflag = 1;
		(void)printf("       mask: %s\n", routename(mask));
		nflag = savenflag;
	}
	if (gate && rtm->rtm_flags & RTF_GATEWAY)
		(void)printf("    gateway: %s\n", routename(gate));
	if (ifp)
		(void)printf("  interface: %.*s\n",
		    ifp->sdl_nlen, ifp->sdl_data);
	(void)printf("      flags: ");
	bprintf(stdout, rtm->rtm_flags, routeflags);

#define lock(f)	((rtm->rtm_rmx.rmx_locks & __CONCAT(RTV_,f)) ? 'L' : ' ')
#define msec(u)	(((u) + 500) / 1000)		/* usec to msec */

	(void) printf("\n%s\n", "\
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire");
	printf("%8u%c ", rtm->rtm_rmx.rmx_recvpipe, lock(RPIPE));
	printf("%8u%c ", rtm->rtm_rmx.rmx_sendpipe, lock(SPIPE));
	printf("%8u%c ", rtm->rtm_rmx.rmx_ssthresh, lock(SSTHRESH));
	printf("%8u%c ", msec(rtm->rtm_rmx.rmx_rtt), lock(RTT));
	printf("%8u%c ", msec(rtm->rtm_rmx.rmx_rttvar), lock(RTTVAR));
	printf("%8u%c ", rtm->rtm_rmx.rmx_hopcount, lock(HOPCOUNT));
	printf("%8u%c ", rtm->rtm_rmx.rmx_mtu, lock(MTU));
	if (rtm->rtm_rmx.rmx_expire)
		rtm->rtm_rmx.rmx_expire -= time(0);
	printf("%8d%c\n", rtm->rtm_rmx.rmx_expire, lock(EXPIRE));
#undef lock
#undef msec
#define	RTA_IGN	(RTA_DST|RTA_GATEWAY|RTA_NETMASK|RTA_IFP|RTA_IFA|RTA_BRD)
	if (verbose)
		pmsg_common(rtm);
	else if (rtm->rtm_addrs &~ RTA_IGN) {
		(void) printf("sockaddrs: ");
		bprintf(stdout, rtm->rtm_addrs, addrnames);
		putchar('\n');
	}
#undef	RTA_IGN
}

void
pmsg_common(rtm)
	register struct rt_msghdr *rtm;
{
	(void) printf("\nlocks: ");
	bprintf(stdout, rtm->rtm_rmx.rmx_locks, metricnames);
	(void) printf(" inits: ");
	bprintf(stdout, rtm->rtm_inits, metricnames);
	pmsg_addrs(((char *)(rtm + 1)), rtm->rtm_addrs);
}

void
pmsg_addrs(cp, addrs)
	char	*cp;
	int	addrs;
{
	register struct sockaddr *sa;
	int i;

	if (addrs == 0) {
		(void) putchar('\n');
		return;
	}
	(void) printf("\nsockaddrs: ");
	bprintf(stdout, addrs, addrnames);
	(void) putchar('\n');
	for (i = 1; i; i <<= 1)
		if (i & addrs) {
			sa = (struct sockaddr *)cp;
			(void) printf(" %s", routename(sa));
			ADVANCE(cp, sa);
		}
	(void) putchar('\n');
	(void) fflush(stdout);
}

void
bprintf(fp, b, s)
	register FILE *fp;
	register int b;
	register u_char *s;
{
	register int i;
	int gotsome = 0;

	if (b == 0)
		return;
	while ((i = *s++) != 0) {
		if (b & (1 << (i-1))) {
			if (gotsome == 0)
				i = '<';
			else
				i = ',';
			(void) putc(i, fp);
			gotsome = 1;
			for (; (i = *s) > 32; s++)
				(void) putc(i, fp);
		} else
			while (*s > 32)
				s++;
	}
	if (gotsome)
		(void) putc('>', fp);
}

int
keyword(cp)
	char *cp;
{
	register struct keytab *kt = keywords;

	while (kt->kt_cp && strcmp(kt->kt_cp, cp))
		kt++;
	return kt->kt_i;
}

/* States*/
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

void
sockaddr(addr, sa)
	register char *addr;
	register struct sockaddr *sa;
{
	register char *cp = (char *)sa;
	int size = sa->sa_len;
	char *cplim = cp + size;
	register int byte = 0, state = VIRGIN, new = 0 /* foil gcc */;

	bzero(cp, size);
	cp++;
	do {
		if ((*addr >= '0') && (*addr <= '9')) {
			new = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			new = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			new = *addr - 'A' + 10;
		} else if (*addr == 0)
			state |= END;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case GOTTWO | DIGIT:
			*cp++ = byte; /*FALLTHROUGH*/
		case VIRGIN | DIGIT:
			state = GOTONE; byte = new; continue;
		case GOTONE | DIGIT:
			state = GOTTWO; byte = new + (byte << 4); continue;
		default: /* | DELIM */
			state = VIRGIN; *cp++ = byte; byte = 0; continue;
		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte; /* FALLTHROUGH */
		case VIRGIN | END:
			break;
		}
		break;
	} while (cp < cplim);
	sa->sa_len = cp - (char *)sa;
}
