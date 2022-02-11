
These are the Transparent Proxying patches for Linux kernel 2.6.

The latest version can always be found at

  http://www.balabit.com/download/files/tproxy/


What does the term 'proxy' mean?
--------------------------------

   A proxy is a server-like program, receiving requests from clients,
   forwarding those requests to the real server on behalf of users,
   and returning the response as it arrives.

   Proxies read and parse the application protocol, and reject invalid
   traffic. As most attacks violate the application protocol, disallowing
   protocol violations usually protects against attacks.

What is transparent proxying?
-----------------------------

   To simplify management tasks of clients sitting behind proxy
   firewalls, the technique 'transparent proxying' was invented.
   Transparent proxying means that the presence of the proxy is invisible
   to the user. Transparent proxying however requires kernel support.

We have a 'REDIRECT' target, isn't that enough?
----------------------------------------------

   Real transparent proxying requires the following three features from
   the IP stack of the computer it is running on:
    1. Redirect sessions destined to the outer network to a local process
       using a packet filter rule.
    2. Make it possible for a process to listen to connections on a
       foreign address.
    3. Make it possible for a process to initiate a connection with a
       foreign address as a source.

   Item #1 is usually provided by packet filtering packages like
   Netfilter/IPTables, IPFilter. (yes, this is the REDIRECT target)

   All three were provided in Linux kernels 2.2.x, but support for this
   was removed.

How to install it?
------------------

   Download the latest tproxy-kernel-<kernelversion>*.tar.bz2 tarball
   for your kernel (from v2.6.24),  with the tproxy-iptables-*.patch file.
   
   Patch your kernel using:

      cd /usr/src/linux
      cat <path_to_tproxy>/00*.patch | patch -p1

   then enable tproxy support, `socket' and `TPROXY' modules
   (with optional conntrack support if you need SNAT), compile your kernel
   and  modules.

   The required modules are automatically loaded if the iptables commands
   are used.

   The IPtables patches:

      cd /usr/src/iptables-1.4.X
      cat <path_to_tproxy>/tproxy-iptables*.patch | patch -p1
  
   then compile it on the usual way:
 
      ./autogen.sh
      ./configure && make && make install

   Squid-3 has official support of TProxy v4.1:

   checkout the source code of squid-3 as in
  
      http://wiki.squid-cache.org/Squid3VCS


   then compile it:

      cd ~/source/squid
      ./bootstrap.sh
      ./configure --enable-linux-netfilter && make && make install
 
   Of course you might need to change the path in the examples above.

How to start using it?
----------------------

   This implementation of transparent proxying works by marking packets and
   changing the route based on packet mark. The foreign address bind and tproxy 
   redirection is enabled via a new socket option, IP_TRANSPARENT, without it
   neither the bind nor the tproxy target works.

   Now let's see what happens when a proxy tries to use the required tproxy
   features I outlined earlier.

   1. Redirection

     This is easy, as this was already supported by iptables. Redirection is
     equivalent with the following nat rule:

       iptables -t nat -A PREROUTING -j DNAT --to-dest <localip> --to-port <proxyport>

         <localip>   is one the IP address of the interface where the packet
                     entered the IP stack
         <proxyport> is the port where the proxy was bound to

     To indicate that this is not simple NAT rule, a separate target, 'TPROXY'
     was created:

       iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY --on-port <proxyport>  \
              --tproxy-mark 0x1/0x1

     The local IP address is determined automatically, but can be overridden
     by the --on-ip parameter.

     The marked sockets has to be routed locally:

        ip rule add fwmark 1 lookup 100
        ip route add local 0.0.0.0/0 dev lo table 100


   2. Listening for connections on a foreign address

     There are protocols which use more than a single TCP channel for
     communication. The best example is FTP which uses a command channel for
     sending commands, and a data channel to transfer the body of files. The
     secondary channel can be established in both active and passive mode, 
     active meaning the server connects back to the client, passive meaning
     the client connects to the server on another port.

     Let's see the passive case, when the client establishes a connection to
     the address returned in the response of the PASV FTP command.

     As the presence of the proxy is transparent to the client, the target
     IP address of the secondary channel (e.g. the address in the PASV
     response) is the server (and not the firewall) and this connection must
     also be handled by the proxy. 

     The first solution that comes to mind is to add a a TPROXY rule
     automatically (e.g. to redirect a connection destined to a given server
     on a given port to a local process), however it is not feasible, adding
     rules on the fly should not be required as it would mess the
     administrator's own rules, the NAT translation should be done
     implicitly without touching the user rulebase.

     To do this on a Linux 2.2 kernel it was enough to call bind() on a
     socket with a foreign IP address, and if a new connection to the given
     foreign IP was routed through the firewall the connection was
     intercepted. This solution however distracted the core network kernel
     hackers and removed this feature. This implementation is similar to
     the old behaviour although it works a bit differently:

       * the proxy sets the IP_TRANSPARENT socket option on the listening
         socket
       * the proxy then binds to the foreign address
       * the proxy accepts incoming connections

     It requires additional iptables rules with the socket module of the
     tproxy patches:

        iptables -t mangle -N DIVERT
        iptables -t mangle -A PREROUTING -p tcpo -m socket -j DIVERT
        iptables -t mangle -A DIVERT -j MARK --set-xmark 0x1/0xffffffff
        iptables -t mangle -A DIVERT -j ACCEPT

    the best if the second rule is before using the TPROXY target.

   3. Initiating connections with a foreign address as a source

     Similarly to the case outlined above, it is sometimes necessary to be
     able to initiate a connection with a foreign IP address as a source. 
     Imagine the active FTP case when the FTP client listens for connections
     with source address equal to the server. Another example: a webserver
     in your DMZ which does access control based on client IP address. If
     the proxy could not initiate connections with foreign IP address, the
     webserver would see the inner IP address of the firewall itself.

     In Linux 2.2 this was accomplished by bind()-ing to a foreign address
     prior calling connect(), and it worked. In this tproxy patch it is done
     somewhat similar to the case 2 outlined above.

       * the proxy calls setsockopt with IP_TRANSPARENT

       * the proxy bind to a foreign address

       * the tproxy calls connect()

     The iptables rules with the socket match are also required here.
 
How to use it?
--------------

    The following use-case assumes a transparent proxy listening on port
    50080 and any ip address (0.0.0.0).

    First, set up the routing rules with iproute2:

      ip rule add fwmark 1 lookup 100
      ip route add local 0.0.0.0/0 dev lo table 100

    Or, if you want to use packet marking for anything else, the least
    significant bit is enough for transparent proxying.

      ip rule add fwmark 0x1/0x1 lookup 100
      ip route add local 0.0.0.0/0 dev lo table 100

    Note that this latter example is only working with newer versions of
    iproute2.

    For supporting foreign address bind, the socket match is required with
    packet marking:

      iptables -t mangle -N DIVERT
      iptables -t mangle -A PREROUTING -p tcp -m socket -j DIVERT

      # DIVERT chain: mark packets and accept
      iptables -t mangle -A DIVERT -j MARK --set-mark 1
      iptables -t mangle -A DIVERT -j ACCEPT

    The last rule is for diverting traffic to the proxy:
      
      iptables -t mangle -A PREROUTING -p tcp --dport 80 -j TPROXY \
              --tproxy-mark 0x1/0x1 --on-port 50080

    If it is a Squid-3 proxy, in /etc/squid/squid.conf the following
    rule is necessary for transparent proxying:

      http_port 50080 tproxy transparent

    Then set up the ACL rules according to your local policy.

