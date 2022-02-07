FROM mvance/unbound:latest

LABEL maintainer="AmneziaVPN" 

RUN echo " \n\
domain-insecure: \"coin.\"\n\
domain-insecure: \"emc.\"\n\
domain-insecure: \"lib.\"\n\
domain-insecure: \"bazar.\"\n\
domain-insecure: \"enum.\"\n\
\n\
stub-zone:\n\
   name: coin.\n\
   stub-host: seed1.emercoin.com\n\
   stub-host: seed2.emercoin.com\n\
   stub-first: yes\n\
\n\
stub-zone:\n\
   name: emc.\n\
   stub-host: seed1.emercoin.com\n\
   stub-host: seed2.emercoin.com\n\
   stub-first: yes\n\
\n\
stub-zone:\n\
   name: lib.\n\
   stub-host: seed1.emercoin.com\n\
   stub-host: seed2.emercoin.com\n\
   stub-first: yes\n\
\n\
stub-zone:\n\
   name: bazar.\n\
   stub-host: seed1.emercoin.com\n\
   stub-host: seed2.emercoin.com\n\
   stub-first: yes\n\
\n\
stub-zone:\n\
   name: enum.\n\
   stub-host: seed1.emercoin.com\n\
   stub-host: seed2.emercoin.com\n\
   stub-first: yes\n\
\n\
forward-zone:\n\
   name: .\n\
   forward-tls-upstream: yes\n\
   forward-addr: 1.1.1.1@853  #cloudflare-dns.com\n\
   forward-addr: 1.0.0.1@853  #cloudflare-dns.com\n\
" | tee /opt/unbound/etc/unbound/forward-records.conf

