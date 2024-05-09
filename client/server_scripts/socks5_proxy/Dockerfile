FROM 3proxy/3proxy:latest

LABEL maintainer="AmneziaVPN" 

RUN mkdir -p /opt/amnezia
RUN echo -e "#!/bin/bash\ntail -f /dev/null" > /opt/amnezia/start.sh
RUN chmod a+x /opt/amnezia/start.sh

ENTRYPOINT [ "/bin/sh", "/opt/amnezia/start.sh" ]
CMD [ "" ]