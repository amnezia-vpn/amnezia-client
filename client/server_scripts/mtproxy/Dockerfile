FROM amneziavpn/mtproxy:latest

RUN mkdir -p /opt/amnezia /data
RUN printf '#!/bin/sh\ntail -f /dev/null\n' > /opt/amnezia/start.sh && \
    chmod a+x /opt/amnezia/start.sh

VOLUME /data
ENTRYPOINT ["/bin/sh", "/opt/amnezia/start.sh"]
CMD [""]
