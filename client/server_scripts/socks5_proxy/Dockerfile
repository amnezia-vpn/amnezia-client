FROM snail007/goproxy:latest

LABEL maintainer="AmneziaVPN" 

#Install required packages
RUN apk add --no-cache bash

RUN mkdir -p /opt/amnezia
RUN echo -e "#!/bin/bash\ntail -f /dev/null" > /opt/amnezia/start.sh
RUN chmod a+x /opt/amnezia/start.sh

ENTRYPOINT [ "/bin/sh", "/opt/amnezia/start.sh" ]
CMD [ "" ]
