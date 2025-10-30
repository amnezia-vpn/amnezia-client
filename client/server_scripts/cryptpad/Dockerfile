FROM promasu/cryptpad:latest

# Install nginx and openssl for HTTPS termination
# Assuming Debian/Ubuntu base. If the base image differs, package manager may need adjustment.
RUN apt-get update \
    && apt-get install -y --no-install-recommends nginx-light openssl \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /etc/nginx/ssl /etc/nginx/conf.d

