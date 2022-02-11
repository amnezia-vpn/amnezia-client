from functools import partial
from multiprocessing.dummy import Pool as ThrPool
from subprocess import check_call, check_output, STDOUT
import multiprocessing
import os
import time

import pytest

# 10.0.1.0/24 (br-web)  -- web, squid, inetd, gw
# 10.0.2.0/24 (br-tank) -- tank, openwrt
# 10.0.8.0/24 (br-txrx) -- gw, openwrt

BR = {
    '10.0.1': 'web',
    '10.0.8': 'txrx',
    '10.0.2': 'tank',
}

GW = {
    'web':  '10.0.1.1',
    'txrx': '10.0.8.1',
    'tank': '10.0.2.123',
}

SLEEPING_BEAST = 'sleep 3600'
DOCKER_CMD = os.environ.get('DOCKER_CMD', '/usr/bin/docker')
PIPEWORK_CMD = os.environ.get('PIPEWORK_CMD', '/usr/bin/pipework')

class VM(object):
    def __init__(self, name, tag, ip4=None, cmd='', docker_opt=''):
        self.docker = DOCKER_CMD
        self.pipework = PIPEWORK_CMD
        self.netns = '/var/run/netns'
        self.dns = '8.8.8.8'

        self.name, self.tag = name, tag
        self.cmd, self.docker_opt = cmd, docker_opt
        if ip4:
            self.ip4 = ip4
        self.sha = self.output('sudo docker run --detach --dns {dns} --name {name} --hostname {name} {docker_opt} {tag} {cmd}')
        self.pid = int(self.output('docker inspect -f {{{{.State.Pid}}}} {sha}'))
        if not os.path.exists(self.netns):
            self.call('sudo mkdir {netns}')
        self.call('sudo ln -sf /proc/{pid}/ns/net {netns}/{name}')
        self.net()
        while cmd != SLEEPING_BEAST and 'LISTEN' not in self.do('netstat -ltn'):
            time.sleep(0.1)

    def net(self):
        self.net_noext()
        self.net_br()

    def net_br(self):
        self.net_br_gw()

    def net_noext(self):
        self.netcall('ip link set dev eth0 down')
        self.netcall('ip route replace unreachable {dns}/32')

    def net_br_gw(self):
        self.call('sudo {pipework} br-{br} -i {intif} -l {vethif} {name} {ip4}/24@{gw4}')

    def net_br_nogw(self):
        self.call('sudo {pipework} br-{br} -i {intif} -l {vethif} {name} {ip4}/24')

    @property
    def br(self):
        return BR[self.ip4.rsplit('.', 1)[0]]
    @property
    def gw4(self):
        return GW[self.br]
    @property
    def intif(self):
        return {'web': 'ethw', 'tank': 'etht', 'txrx': 'ethx'}[self.br]
    @property
    def vethif(self):
        return ('v' + self.intif + self.name)[:15] # IFNAMSIZ 16

    def assert_good_death(self):
        pass

    def close(self):
        if hasattr(self, 'sha'):
            self.call('sudo docker stop --time 1 {sha}')
            self.assert_good_death()
            if not getattr(self, 'preserve_root', False):
                self.call('sudo docker rm {sha}')
            del self.sha
    def fmt(self, cmd):
        ctx = self.__dict__.copy()
        for i in xrange(len(dir(self))):
            try:
                ret = cmd.format(**ctx).split()
                break
            except KeyError, e:
                key = e.args[0]
                ctx[key] = getattr(self, key)
        return ret
    def output(self, cmd, **kwargs):
        return check_output(self.fmt(cmd), **kwargs)
    def logs(self, since=None):
        out = self.output('sudo docker logs {sha}', stderr=STDOUT)
        out = [_.split(None, 1) for _ in out.split('\n')]
        out = [(float(_[0]), _[1]) for _ in out if len(_) == 2 and _[0][:10].isdigit()]
        if since is not None:
            out = [_ for _ in out if _[0] >= since]
        return out
    def call(self, cmd):
        check_call(self.fmt(cmd))
    def do(self, cmd):
        return self.output('sudo docker exec {sha} ' + cmd)
    def netcall(self, cmd):
        return self.output('sudo ip netns exec {name} ' + cmd)
    def lsfd(self):
        out = self.output('sudo lsof -p {pid} -F f')
        out = [int(_[1:]) for _ in out.split() if _[0] == 'f' and _[1:].isdigit()]
        assert len(out) > 0
        return out

class WebVM(VM):
    def __init__(self):
        VM.__init__(self, 'web', 'redsocks/web', '10.0.1.80')

class InetdVM(VM):
    def __init__(self):
        VM.__init__(self, 'inetd', 'redsocks/inetd', '10.0.1.13')

class SquidVM(VM):
    def __init__(self, no):
        VM.__init__(self, 'squid-%d' % no, 'redsocks/squid', '10.0.1.%d' % no,
                    docker_opt='--ulimit nofile=65535:65535',
                    cmd='/etc/squid3/squid-%d.conf' % no)
    def net(self):
        self.net_br_nogw()
        self.netcall('ip route replace 10.0.0.0/16 via 10.0.1.1')

class DanteVM(VM):
    def __init__(self, no):
        VM.__init__(self, 'dante-%d' % no, 'redsocks/dante', '10.0.1.%d' % (180 + no),
                    cmd='/etc/danted-%d.conf' % (1080 + no))
    def net(self):
        self.net_br_nogw()
        self.netcall('ip route replace 10.0.0.0/16 via 10.0.1.1')


class GwVM(VM):
    def __init__(self):
        VM.__init__(self, 'gw', 'ubuntu:14.04', cmd=SLEEPING_BEAST)
    def net_br(self):
        self.ip4 = '10.0.1.1'
        self.net_br_nogw()
        self.ip4 = '10.0.8.1'
        self.net_br_nogw()
        del self.ip4
        self.netcall('ip route replace unreachable 10.0.2.0/24')
        self.netcall('iptables -A FORWARD --destination 10.0.1.66/32 -j DROP')

class TankVM(VM):
    def __init__(self, no):
        assert 1 <= no <= 100
        VM.__init__(self, 'tank%d' % no, 'redsocks/tank', '10.0.2.%d' % no, cmd=SLEEPING_BEAST)

class RegwVM(VM):
    def __init__(self):
        kw = {}
        self.debug = os.environ.get('DEBUG_TEST', '')
        if self.debug:
            self.preserve_root = True
            if self.debug != 'logs':
                kw['cmd'] = {
                    'valgrind': 'valgrind --leak-check=full --show-leak-kinds=all /usr/local/sbin/redsocks -c /usr/local/etc/redsocks.conf',
                    'strace': 'strace -ttt /usr/local/sbin/redsocks -c /usr/local/etc/redsocks.conf',
                }[self.debug]
        VM.__init__(self, 'regw', 'redsocks/regw', **kw)
    def net_br(self):
        self.ip4 = '10.0.2.123'
        self.net_br_nogw()
        self.ip4 = '10.0.8.123'
        self.net_br_gw()
        del self.ip4
        for t in TANKS.values():
            self.netcall('iptables -t nat -A PREROUTING --source 10.0.2.%d/32 --dest 10.0.1.0/24 -p tcp -j REDIRECT --to-port %d' % (t, 12340 + t - TANKS_BASE))
    def assert_good_death(self):
        out = self.output('sudo docker logs {sha}', stderr=STDOUT)
        assert 'redsocks goes down' in out and 'There are connected clients during shutdown' not in out
        assert self.debug != 'valgrind' or 'no leaks are possible' in out

CPU = object()
MAX = object()
def pmap(l, j=MAX):
    #return map(lambda x: x(), l)
    if j is MAX:
        j = len(l)
    elif j is CPU:
        j = multiprocessing.cpu_count()
    p = ThrPool(j)
    try:
        return p.map(lambda x: x(), l, chunksize=1)
    finally:
        p.close()
        p.join()

TANKS_BASE = 10
TANKS = {
    'connect_none': TANKS_BASE + 0,
    'connect_basic': TANKS_BASE + 1,
    'connect_digest': TANKS_BASE + 2,
    'socks5_none': TANKS_BASE + 3,
    'socks5_auth': TANKS_BASE + 4,
    'connect_nopass': TANKS_BASE + 5,
    'connect_baduser': TANKS_BASE + 6,
    'connect_badpass': TANKS_BASE + 7,
    'socks5_nopass': TANKS_BASE + 8,
    'socks5_baduser': TANKS_BASE + 9,
    'socks5_badpass': TANKS_BASE + 10,
    'httperr_connect_nopass': TANKS_BASE + 11,
    'httperr_connect_baduser': TANKS_BASE + 12,
    'httperr_connect_badpass': TANKS_BASE + 13,
    'httperr_connect_digest': TANKS_BASE + 14,
    'httperr_proxy_refuse': TANKS_BASE + 15,
    'httperr_proxy_timeout': TANKS_BASE + 16,
}

class _Network(object):
    def __init__(self):
        assert os.path.exists(PIPEWORK_CMD)
        running = check_output('sudo docker ps -q'.split()).split()
        assert running == []
        names = check_output('sudo docker ps -a --format {{.Names}}'.split()).split()
        assert 'regw' not in names
        vm = [
            GwVM,
            WebVM,
            InetdVM,
            RegwVM,
            partial(SquidVM, 8),
            partial(SquidVM, 9),
            partial(DanteVM, 0),
            partial(DanteVM, 1),
        ]
        for t in TANKS.values():
            vm.append(partial(TankVM, t))
        self.vm = {_.name: _ for _ in pmap(vm, j=CPU)} # pmap saves ~5 seconds
    def close(self):
        check_output('sudo docker ps'.split())
        pmap([_.close for _ in self.vm.values()]) # pmap saves ~7 seconds

@pytest.fixture(scope="session")
def net(request):
    n = _Network()
    request.addfinalizer(n.close)
    return n

def pytest_addoption(parser):
    parser.addoption('--vmdebug', action='store_true', help='run `test_debug` test')

def pytest_cmdline_preparse(args):
    if '--vmdebug' in args:
        args[:] = ['-k', 'test_vmdebug'] + args
