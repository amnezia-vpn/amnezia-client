#!/usr/bin/python2

import argparse
import functools
import socket
import logging
import json
import time
import datetime
import sys

import tornado.ioloop as ioloop
import tornado.iostream as iostream
import tornado.httpclient as httpclient

log = logging.getLogger('benchmark')

conlist = []
stats = []

DELAY = 0.1

class Mode(object):
    def __init__(self, opt):
        self.opt = opt
        conlist.append(self)
        log.debug('%d of %d', len(conlist), opt.stop)
        self.start()
    def grab_stats(self):
        if self.opt.json is not None:
            client = httpclient.AsyncHTTPClient()
            self.begin_stats = time.time()
            client.fetch('http://%s/debug/meminfo.json' % self.opt.json, self.parse_stats)
        else:
            self.schedule_next()
    def parse_stats(self, response):
        log.debug('got stats')
        self.end_stats = time.time()
        sdict = json.loads(response.body)
        sdict['begin'] = self.begin_stats
        sdict['end'] = self.end_stats
        sdict['conns'] = len(conlist)
        stats.append(sdict)
        self.schedule_next()
    def schedule_next(self):
        if len(conlist) < self.opt.stop:
            ioloop.IOLoop.current().add_timeout(datetime.timedelta(seconds=DELAY), functools.partial(self.__class__, self.opt))
        else:
            ioloop.IOLoop.current().stop()

class Pipe(Mode):
    def start(self):
        client = httpclient.AsyncHTTPClient()
        self.begin_stats = time.time()
        client.fetch('http://%s/debug/pipe' % self.opt.json, self.on_pipe)
    def on_pipe(self, response):
        self.grab_stats()

class Connector(Mode):
    def start(self):
        fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        self.stream = iostream.IOStream(fd)
        self.stream.connect((opt.host, opt.port), self.on_connect)

class EchoConnector(Connector):
    ECHO = 'ECHO\x0d\x0a'
    def on_connect(self):
        self.stream.write(self.ECHO)
        self.stream.read_bytes(len(self.ECHO), self.on_pong)
    def on_pong(self, data):
        assert data == self.ECHO
        self.grab_stats()

class ChargenConnector(Connector):
    def __init__(self, opt):
        Connector.__init__(self, opt)
        self.gotstats = False
    def on_connect(self):
        self.stream.read_until_close(self.on_close, self.on_datachunk)
    def on_close(self, data):
        pass
    def on_datachunk(self, data):
        if not self.gotstats:
            self.gotstats = True
            self.grab_stats()

MODES = {
    'echo': EchoConnector,
    'chargen': ChargenConnector,
    'pipe': Pipe,
}

def parse_cmd(argv):
    parser = argparse.ArgumentParser(prog=argv[0], description='/ping handler')
    parser.add_argument('--json', type=str)
    parser.add_argument('--stop', default=10, type=int)
    parser.add_argument('--verbose', default=False, action='store_true')
    parser.add_argument('--mode', metavar='MODE', required=True, choices=sorted(MODES))
    parser.add_argument('--port', metavar='PORT', type=int, default=None)
    parser.add_argument('host', metavar='HOST', type=str)
    if len(sys.argv) == 1:
        sys.argv.append('-h')
    opt = parser.parse_args(argv[1:])
    if opt.port is None:
        opt.port = {'echo': 7, 'chargen': 19, 'pipe': -1}[opt.mode]
    return opt

def main(opt):
    LOGGING_FORMAT = '%(asctime)s %(name)s[%(process)d] %(levelname)s: %(message)s'
    if not opt.verbose:
        logging.basicConfig(level=logging.INFO, format=LOGGING_FORMAT)
        logging.getLogger('tornado.access').setLevel(logging.INFO+1)
    else:
        logging.basicConfig(level=logging.DEBUG, format=LOGGING_FORMAT)

    klass = MODES[opt.mode]
    klass(opt)

    try:
        ioloop.IOLoop.instance().start()
    except KeyboardInterrupt:
        pass
    finally:
        log.fatal('Done.')
    print json.dumps(stats)

if __name__ == '__main__':
    main(parse_cmd(sys.argv))
