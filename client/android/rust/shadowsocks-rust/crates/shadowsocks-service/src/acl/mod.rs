//! Access Control List (ACL) for shadowsocks
//!
//! This is for advance controlling server behaviors in both local and proxy servers.

use std::{
    fmt,
    fs::File,
    io::{self, BufRead, BufReader, Error, ErrorKind},
    net::{IpAddr, SocketAddr},
    path::Path,
};

use ipnet::{IpNet, Ipv4Net, Ipv6Net};
use iprange::IpRange;
use regex::{RegexSet, RegexSetBuilder};

use shadowsocks::{context::Context, relay::socks5::Address};

/// Strategy mode that ACL is running
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Mode {
    /// BlackList mode, rejects or bypasses all requests by default
    BlackList,
    /// WhiteList mode, accepts or proxies all requests by default
    WhiteList,
}

#[derive(Clone)]
struct Rules {
    ipv4: IpRange<Ipv4Net>,
    ipv6: IpRange<Ipv6Net>,
    rule: RegexSet,
}

impl fmt::Debug for Rules {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Rules {{ ipv4: {:?}, ipv6: {:?}, rule: [", self.ipv4, self.ipv6)?;

        let max_len = 2;
        let has_more = self.rule.len() > max_len;

        for (idx, r) in self.rule.patterns().iter().take(max_len).enumerate() {
            if idx > 0 {
                f.write_str(", ")?;
            }
            f.write_str(r)?;
        }

        if has_more {
            f.write_str(", ...")?;
        }

        f.write_str("] }")
    }
}

impl Rules {
    /// Create a new rule
    fn new(mut ipv4: IpRange<Ipv4Net>, mut ipv6: IpRange<Ipv6Net>, rule: RegexSet) -> Rules {
        // Optimization, merging networks
        ipv4.simplify();
        ipv6.simplify();

        Rules { ipv4, ipv6, rule }
    }

    /// Check if the specified address matches these rules
    #[allow(dead_code)]
    fn check_address_matched(&self, addr: &Address) -> bool {
        match *addr {
            Address::SocketAddress(ref saddr) => self.check_ip_matched(&saddr.ip()),
            Address::DomainNameAddress(ref domain, ..) => self.check_host_matched(domain),
        }
    }

    /// Check if the specified address matches any rules
    fn check_ip_matched(&self, addr: &IpAddr) -> bool {
        match addr {
            IpAddr::V4(v4) => self.ipv4.contains(v4),
            IpAddr::V6(v6) => self.ipv6.contains(v6),
        }
    }

    /// Check if the specified host matches any rules
    fn check_host_matched(&self, host: &str) -> bool {
        self.rule.is_match(host)
    }

    /// Check if there are no rules for IP addresses
    fn is_ip_empty(&self) -> bool {
        self.ipv4.is_empty() && self.ipv6.is_empty()
    }

    /// Check if there are no rules for domain names
    fn is_host_empty(&self) -> bool {
        self.rule.len() == 0
    }
}

/// ACL rules
///
/// ## Sections
///
/// ACL File is formatted in sections, each section has a name with surrounded by brackets `[` and `]`
/// followed by Rules line by line.
///
/// ```plain
/// [SECTION-1]
/// RULE-1
/// RULE-2
/// RULE-3
///
/// [SECTION-2]
/// RULE-1
/// RULE-2
/// RULE-3
/// ```
///
/// Available sections are
///
/// - For local servers (`sslocal`, `ssredir`, ...)
///     * `[bypass_all]` - ACL runs in `BlackList` mode.
///     * `[proxy_all]` - ACL runs in `WhiteList` mode.
///     * `[bypass_list]` - Rules for connecting directly
///     * `[proxy_list]` - Rules for connecting through proxies
/// - For remote servers (`ssserver`)
///     * `[reject_all]` - ACL runs in `BlackList` mode.
///     * `[accept_all]` - ACL runs in `WhiteList` mode.
///     * `[black_list]` - Rules for rejecting
///     * `[white_list]` - Rules for allowing
///     * `[outbound_block_list]` - Rules for blocking outbound addresses.
///
/// ## Mode
///
/// Mode is the default ACL strategy for those addresses that are not in configuration file.
///
/// - `BlackList` - Bypasses / Rejects all addresses except those in `[proxy_list]` or `[white_list]`
/// - `WhiltList` - Proxies / Accepts all addresses except those in `[bypass_list]` or `[black_list]`
///
/// ## Rules
///
/// Rules can be either
///
/// - CIDR form network addresses, like `10.9.0.32/16`
/// - IP addresses, like `127.0.0.1` or `::1`
/// - Regular Expression for matching hosts, like `(^|\.)gmail\.com$`
#[derive(Debug, Clone)]
pub struct AccessControl {
    outbound_block: Rules,
    black_list: Rules,
    white_list: Rules,
    mode: Mode,
}

impl AccessControl {
    /// Load ACL rules from a file
    pub fn load_from_file<P: AsRef<Path>>(p: P) -> io::Result<AccessControl> {
        let fp = File::open(p)?;
        let r = BufReader::new(fp);

        let mut mode = Mode::BlackList;

        let mut outbound_block_ipv4 = IpRange::new();
        let mut outbound_block_ipv6 = IpRange::new();
        let mut outbound_block_rules = Vec::new();
        let mut bypass_ipv4 = IpRange::new();
        let mut bypass_ipv6 = IpRange::new();
        let mut bypass_rules = Vec::new();
        let mut proxy_ipv4 = IpRange::new();
        let mut proxy_ipv6 = IpRange::new();
        let mut proxy_rules = Vec::new();

        let mut curr_ipv4 = &mut bypass_ipv4;
        let mut curr_ipv6 = &mut bypass_ipv6;
        let mut curr_rules = &mut bypass_rules;

        for line in r.lines() {
            let line = line?;
            if line.is_empty() {
                continue;
            }

            // Comments
            if line.starts_with('#') {
                continue;
            }

            match line.as_str() {
                "[reject_all]" | "[bypass_all]" => {
                    mode = Mode::WhiteList;
                }
                "[accept_all]" | "[proxy_all]" => {
                    mode = Mode::BlackList;
                }
                "[outbound_block_list]" => {
                    curr_ipv4 = &mut outbound_block_ipv4;
                    curr_ipv6 = &mut outbound_block_ipv6;
                    curr_rules = &mut outbound_block_rules;
                }
                "[black_list]" | "[bypass_list]" => {
                    curr_ipv4 = &mut bypass_ipv4;
                    curr_ipv6 = &mut bypass_ipv6;
                    curr_rules = &mut bypass_rules;
                }
                "[white_list]" | "[proxy_list]" => {
                    curr_ipv4 = &mut proxy_ipv4;
                    curr_ipv6 = &mut proxy_ipv6;
                    curr_rules = &mut proxy_rules;
                }
                _ => {
                    match line.parse::<IpNet>() {
                        Ok(IpNet::V4(v4)) => {
                            curr_ipv4.add(v4);
                        }
                        Ok(IpNet::V6(v6)) => {
                            curr_ipv6.add(v6);
                        }
                        Err(..) => {
                            // Maybe it is a pure IpAddr
                            match line.parse::<IpAddr>() {
                                Ok(IpAddr::V4(v4)) => {
                                    curr_ipv4.add(Ipv4Net::from(v4));
                                }
                                Ok(IpAddr::V6(v6)) => {
                                    curr_ipv6.add(Ipv6Net::from(v6));
                                }
                                Err(..) => {
                                    // FIXME: If this line is not a valid regex, how can we know without actually compile it?
                                    curr_rules.push(line);
                                }
                            }
                        }
                    }
                }
            }
        }

        const REGEX_SIZE_LIMIT: usize = usize::max_value();

        let outbound_block_regex = match RegexSetBuilder::new(outbound_block_rules)
            .size_limit(REGEX_SIZE_LIMIT)
            .build()
        {
            Ok(r) => r,
            Err(err) => {
                let err = Error::new(ErrorKind::Other, format!("[outbound_block_list] regex error: {}", err));
                return Err(err);
            }
        };

        let bypass_regex = match RegexSetBuilder::new(bypass_rules)
            .case_insensitive(true)
            .size_limit(REGEX_SIZE_LIMIT)
            .build()
        {
            Ok(r) => r,
            Err(err) => {
                let err = Error::new(
                    ErrorKind::Other,
                    format!("[black_list] or [bypass_list] regex error: {}", err),
                );
                return Err(err);
            }
        };

        let proxy_regex = match RegexSetBuilder::new(proxy_rules)
            .case_insensitive(true)
            .size_limit(REGEX_SIZE_LIMIT)
            .build()
        {
            Ok(r) => r,
            Err(err) => {
                let err = Error::new(
                    ErrorKind::Other,
                    format!("[white_list] or [proxy_list] regex error: {}", err),
                );
                return Err(err);
            }
        };

        Ok(AccessControl {
            outbound_block: Rules::new(outbound_block_ipv4, outbound_block_ipv6, outbound_block_regex),
            black_list: Rules::new(bypass_ipv4, bypass_ipv6, bypass_regex),
            white_list: Rules::new(proxy_ipv4, proxy_ipv6, proxy_regex),
            mode,
        })
    }

    /// Check if domain name is in proxy_list.
    /// If so, it should be resolved from remote (for Android's DNS relay)
    ///
    /// Return
    /// - `Some(true)` if `host` is in `white_list` (should be proxied)
    /// - `Some(false)` if `host` is in `black_list` (should be bypassed)
    /// - `None` if `host` doesn't match any rules
    pub fn check_host_in_proxy_list(&self, host: &str) -> Option<bool> {
        // Addresses in proxy_list will be proxied
        if self.white_list.check_host_matched(host) {
            return Some(true);
        }
        // Addresses in bypass_list will be bypassed
        if self.black_list.check_host_matched(host) {
            return Some(false);
        }
        None
    }

    /// If there are no IP rules
    pub fn is_ip_empty(&self) -> bool {
        match self.mode {
            Mode::BlackList => self.black_list.is_ip_empty(),
            Mode::WhiteList => self.white_list.is_ip_empty(),
        }
    }

    /// If there are no domain name rules
    pub fn is_host_empty(&self) -> bool {
        self.black_list.is_host_empty() && self.white_list.is_host_empty()
    }

    /// Check if `IpAddr` should be proxied
    pub fn check_ip_in_proxy_list(&self, ip: &IpAddr) -> bool {
        match self.mode {
            Mode::BlackList => !self.black_list.check_ip_matched(ip),
            Mode::WhiteList => self.white_list.check_ip_matched(ip),
        }
    }

    /// Default mode
    ///
    /// Default behavor for hosts that are not configured
    /// - `true` - Proxied
    /// - `false` - Bypassed
    pub fn is_default_in_proxy_list(&self) -> bool {
        match self.mode {
            Mode::BlackList => true,
            Mode::WhiteList => false,
        }
    }

    /// Check if target address should be bypassed (for client)
    ///
    /// This function may perform a DNS resolution
    pub async fn check_target_bypassed(&self, context: &Context, addr: &Address) -> bool {
        match *addr {
            Address::SocketAddress(ref addr) => !self.check_ip_in_proxy_list(&addr.ip()),
            // Resolve hostname and check the list
            Address::DomainNameAddress(ref host, port) => {
                if let Some(value) = self.check_host_in_proxy_list(host) {
                    return !value;
                }
                if self.is_ip_empty() {
                    return !self.is_default_in_proxy_list();
                }
                if let Ok(vaddr) = context.dns_resolve(host, port).await {
                    for addr in vaddr {
                        if !self.check_ip_in_proxy_list(&addr.ip()) {
                            return true;
                        }
                    }
                }
                false
            }
        }
    }

    /// Check if client address should be blocked (for server)
    pub fn check_client_blocked(&self, addr: &SocketAddr) -> bool {
        match self.mode {
            Mode::BlackList => {
                // Only clients in black_list will be blocked
                self.black_list.check_ip_matched(&addr.ip())
            }
            Mode::WhiteList => {
                // Only clients in white_list will be proxied
                !self.white_list.check_ip_matched(&addr.ip())
            }
        }
    }

    /// Check if outbound address is blocked (for server)
    ///
    /// NOTE: `Address::DomainName` is only validated by regex rules,
    ///       resolved addresses are checked in the `lookup_outbound_then!` macro
    pub async fn check_outbound_blocked(&self, context: &Context, outbound: &Address) -> bool {
        match outbound {
            Address::SocketAddress(saddr) => self.outbound_block.check_ip_matched(&saddr.ip()),
            Address::DomainNameAddress(host, port) => {
                if self.outbound_block.check_host_matched(host) {
                    return true;
                }

                if let Ok(vaddr) = context.dns_resolve(host, *port).await {
                    for addr in vaddr {
                        if self.outbound_block.check_ip_matched(&addr.ip()) {
                            return true;
                        }
                    }
                }

                false
            }
        }
    }
}
