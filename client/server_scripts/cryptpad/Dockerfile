# SPDX-FileCopyrightText: 2023 XWiki CryptPad Team <contact@cryptpad.org> and contributors
#
# SPDX-License-Identifier: AGPL-3.0-or-later

# Multistage build to reduce image size and increase security
FROM node:lts-slim AS build

# Create folder for CryptPad
RUN mkdir /cryptpad
WORKDIR /cryptpad

# Copy CryptPad source code to the container
COPY cryptpad_source /cryptpad

RUN sed -i "s@//httpAddress: 'localhost'@httpAddress: '0.0.0.0'@" /cryptpad/config/config.example.js
RUN sed -i "s@installMethod: 'unspecified'@installMethod: 'docker'@" /cryptpad/config/config.example.js

# Install dependencies
RUN npm install --production \
    && npm run install:components

# Create actual CryptPad image
FROM node:lts-slim
ENV DEBIAN_FRONTEND=noninteractive

# Create user and group for CryptPad so it does not run as root
RUN groupadd cryptpad -g 4001
RUN useradd cryptpad -u 4001 -g 4001 -d /cryptpad

# Install curl for healthcheck
# Install git, rdfind and unzip for install-onlyoffice.sh
RUN apt-get update && apt-get install --no-install-recommends -y \
    curl ca-certificates git rdfind unzip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy cryptpad with installed modules
COPY --from=build --chown=cryptpad /cryptpad /cryptpad
USER cryptpad

# Copy docker-entrypoint.sh script
COPY --chown=cryptpad docker-entrypoint.sh /cryptpad/docker-entrypoint.sh

# Set workdir to cryptpad
WORKDIR /cryptpad

# Create directories
RUN mkdir blob block customize data datastore

# Volumes for data persistence
VOLUME /cryptpad/blob
VOLUME /cryptpad/block
VOLUME /cryptpad/customize
VOLUME /cryptpad/data
VOLUME /cryptpad/datastore

ENTRYPOINT ["/bin/bash", "/cryptpad/docker-entrypoint.sh"]

# Healthcheck
HEALTHCHECK --interval=1m CMD curl -f http://localhost:3000/ || exit 1

# Ports
EXPOSE 3000 3003

# Run cryptpad on startup
CMD ["npm", "start"]
