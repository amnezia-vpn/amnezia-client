# Inspired by https://github.com/jitsi/docker-jitsi-meet/blob/master/docker-compose.yml

# version: '3.5'
# 
# services:
#     # Frontend
#     web:
#         image: jitsi/web:${JITSI_IMAGE_VERSION:-unstable}
#         restart: ${RESTART_POLICY:-unless-stopped}
#         ports:
#             - '${HTTP_PORT}:80'
#             - '${HTTPS_PORT}:443'
#         volumes:
#             - ${CONFIG}/web:/config:Z
#             - ${CONFIG}/web/crontabs:/var/spool/cron/crontabs:Z
#             - ${CONFIG}/transcripts:/usr/share/jitsi-meet/transcripts:Z
#         environment:
#             - AMPLITUDE_ID
#             - ANALYTICS_SCRIPT_URLS
#             - ANALYTICS_WHITELISTED_EVENTS
#             - AUDIO_QUALITY_OPUS_BITRATE
#             - AUTO_CAPTION_ON_RECORD
#             - BRANDING_DATA_URL
#             - CALLSTATS_CUSTOM_SCRIPT_URL
#             - CALLSTATS_ID
#             - CALLSTATS_SECRET
#             - CHROME_EXTENSION_BANNER_JSON
#             - COLIBRI_WEBSOCKET_PORT
#             - CONFCODE_URL
#             - CONFIG_EXTERNAL_CONNECT
#             - DEFAULT_LANGUAGE
#             - DEPLOYMENTINFO_ENVIRONMENT
#             - DEPLOYMENTINFO_ENVIRONMENT_TYPE
#             - DEPLOYMENTINFO_REGION
#             - DEPLOYMENTINFO_SHARD
#             - DEPLOYMENTINFO_USERREGION
#             - DESKTOP_SHARING_FRAMERATE_MIN
#             - DESKTOP_SHARING_FRAMERATE_MAX
#             - DIALIN_NUMBERS_URL
#             - DIALOUT_AUTH_URL
#             - DIALOUT_CODES_URL
#             - DISABLE_AUDIO_LEVELS
#             - DISABLE_DEEP_LINKING
#             - DISABLE_GRANT_MODERATOR
#             - DISABLE_HTTPS
#             - DISABLE_KICKOUT
#             - DISABLE_LOCAL_RECORDING
#             - DISABLE_POLLS
#             - DISABLE_PRIVATE_CHAT
#             - DISABLE_PROFILE
#             - DISABLE_REACTIONS
#             - DISABLE_REMOTE_VIDEO_MENU
#             - DISABLE_START_FOR_ALL
#             - DROPBOX_APPKEY
#             - DROPBOX_REDIRECT_URI
#             - DYNAMIC_BRANDING_URL
#             - ENABLE_AUDIO_PROCESSING
#             - ENABLE_AUTH
#             - ENABLE_BREAKOUT_ROOMS
#             - ENABLE_CALENDAR
#             - ENABLE_COLIBRI_WEBSOCKET
#             - ENABLE_E2EPING
#             - ENABLE_FILE_RECORDING_SHARING
#             - ENABLE_GUESTS
#             - ENABLE_HSTS
#             - ENABLE_HTTP_REDIRECT
#             - ENABLE_IPV6
#             - ENABLE_LETSENCRYPT
#             - ENABLE_LIPSYNC
#             - ENABLE_NO_AUDIO_DETECTION
#             - ENABLE_NOISY_MIC_DETECTION
#             - ENABLE_OCTO
#             - ENABLE_OPUS_RED
#             - ENABLE_PREJOIN_PAGE
#             - ENABLE_P2P
#             - ENABLE_WELCOME_PAGE
#             - ENABLE_CLOSE_PAGE
#             - ENABLE_LIVESTREAMING
#             - ENABLE_LOCAL_RECORDING_NOTIFY_ALL_PARTICIPANT
#             - ENABLE_LOCAL_RECORDING_SELF_START
#             - ENABLE_RECORDING
#             - ENABLE_REMB
#             - ENABLE_REQUIRE_DISPLAY_NAME
#             - ENABLE_SERVICE_RECORDING
#             - ENABLE_SIMULCAST
#             - ENABLE_STATS_ID
#             - ENABLE_STEREO
#             - ENABLE_SUBDOMAINS
#             - ENABLE_TALK_WHILE_MUTED
#             - ENABLE_TCC
#             - ENABLE_TRANSCRIPTIONS
#             - ENABLE_XMPP_WEBSOCKET
#             - ENABLE_JAAS_COMPONENTS
#             - ETHERPAD_PUBLIC_URL
#             - ETHERPAD_URL_BASE
#             - E2EPING_NUM_REQUESTS
#             - E2EPING_MAX_CONFERENCE_SIZE
#             - E2EPING_MAX_MESSAGE_PER_SECOND
#             - GOOGLE_ANALYTICS_ID
#             - GOOGLE_API_APP_CLIENT_ID
#             - HIDE_PREMEETING_BUTTONS
#             - HIDE_PREJOIN_DISPLAY_NAME
#             - HIDE_PREJOIN_EXTRA_BUTTONS
#             - INVITE_SERVICE_URL
#             - LETSENCRYPT_DOMAIN
#             - LETSENCRYPT_EMAIL
#             - LETSENCRYPT_USE_STAGING
#             - MATOMO_ENDPOINT
#             - MATOMO_SITE_ID
#             - MICROSOFT_API_APP_CLIENT_ID
#             - NGINX_RESOLVER
#             - NGINX_WORKER_PROCESSES
#             - NGINX_WORKER_CONNECTIONS
#             - PEOPLE_SEARCH_URL
#             - PREFERRED_LANGUAGE
#             - PUBLIC_URL
#             - P2P_PREFERRED_CODEC
#             - RESOLUTION
#             - RESOLUTION_MIN
#             - RESOLUTION_WIDTH
#             - RESOLUTION_WIDTH_MIN
#             - START_AUDIO_MUTED
#             - START_AUDIO_ONLY
#             - START_BITRATE
#             - START_SILENT
#             - START_WITH_AUDIO_MUTED
#             - START_VIDEO_MUTED
#             - START_WITH_VIDEO_MUTED
#             - TESTING_CAP_SCREENSHARE_BITRATE
#             - TESTING_OCTO_PROBABILITY
#             - TOKEN_AUTH_URL
#             - TOOLBAR_BUTTONS
#             - TRANSLATION_LANGUAGES
#             - TRANSLATION_LANGUAGES_HEAD
#             - TZ
#             - USE_APP_LANGUAGE
#             - VIDEOQUALITY_BITRATE_H264_LOW
#             - VIDEOQUALITY_BITRATE_H264_STANDARD
#             - VIDEOQUALITY_BITRATE_H264_HIGH
#             - VIDEOQUALITY_BITRATE_VP8_LOW
#             - VIDEOQUALITY_BITRATE_VP8_STANDARD
#             - VIDEOQUALITY_BITRATE_VP8_HIGH
#             - VIDEOQUALITY_BITRATE_VP9_LOW
#             - VIDEOQUALITY_BITRATE_VP9_STANDARD
#             - VIDEOQUALITY_BITRATE_VP9_HIGH
#             - VIDEOQUALITY_ENFORCE_PREFERRED_CODEC
#             - VIDEOQUALITY_PREFERRED_CODEC
#             - XMPP_AUTH_DOMAIN
#             - XMPP_BOSH_URL_BASE
#             - XMPP_DOMAIN
#             - XMPP_GUEST_DOMAIN
#             - XMPP_MUC_DOMAIN
#             - XMPP_RECORDER_DOMAIN
#             - XMPP_PORT
#             - WHITEBOARD_ENABLED
#             - WHITEBOARD_COLLAB_SERVER_PUBLIC_URL
#         networks:
#             meet.jitsi:
# 
#     # XMPP server
#     prosody:
#         image: jitsi/prosody:${JITSI_IMAGE_VERSION:-unstable}
#         restart: ${RESTART_POLICY:-unless-stopped}
#         expose:
#             - '${XMPP_PORT:-5222}'
#             - '5347'
#             - '5280'
#         volumes:
#             - ${CONFIG}/prosody/config:/config:Z
#             - ${CONFIG}/prosody/prosody-plugins-custom:/prosody-plugins-custom:Z
#         environment:
#             - AUTH_TYPE
#             - DISABLE_POLLS
#             - ENABLE_AUTH
#             - ENABLE_AV_MODERATION
#             - ENABLE_BREAKOUT_ROOMS
#             - ENABLE_END_CONFERENCE
#             - ENABLE_GUESTS
#             - ENABLE_IPV6
#             - ENABLE_LOBBY
#             - ENABLE_RECORDING
#             - ENABLE_XMPP_WEBSOCKET
#             - ENABLE_JAAS_COMPONENTS
#             - GC_TYPE
#             - GC_INC_TH
#             - GC_INC_SPEED
#             - GC_INC_STEP_SIZE
#             - GC_GEN_MIN_TH
#             - GC_GEN_MAX_TH
#             - GLOBAL_CONFIG
#             - GLOBAL_MODULES
#             - JIBRI_RECORDER_USER
#             - JIBRI_RECORDER_PASSWORD
#             - JIBRI_XMPP_USER
#             - JIBRI_XMPP_PASSWORD
#             - JICOFO_AUTH_PASSWORD
#             - JICOFO_COMPONENT_SECRET
#             - JIGASI_XMPP_USER
#             - JIGASI_XMPP_PASSWORD
#             - JVB_AUTH_USER
#             - JVB_AUTH_PASSWORD
#             - JWT_APP_ID
#             - JWT_APP_SECRET
#             - JWT_ACCEPTED_ISSUERS
#             - JWT_ACCEPTED_AUDIENCES
#             - JWT_ASAP_KEYSERVER
#             - JWT_ALLOW_EMPTY
#             - JWT_AUTH_TYPE
#             - JWT_ENABLE_DOMAIN_VERIFICATION
#             - JWT_TOKEN_AUTH_MODULE
#             - MATRIX_UVS_URL
#             - MATRIX_UVS_ISSUER
#             - MATRIX_UVS_AUTH_TOKEN
#             - MATRIX_UVS_SYNC_POWER_LEVELS
#             - LOG_LEVEL
#             - LDAP_AUTH_METHOD
#             - LDAP_BASE
#             - LDAP_BINDDN
#             - LDAP_BINDPW
#             - LDAP_FILTER
#             - LDAP_VERSION
#             - LDAP_TLS_CIPHERS
#             - LDAP_TLS_CHECK_PEER
#             - LDAP_TLS_CACERT_FILE
#             - LDAP_TLS_CACERT_DIR
#             - LDAP_START_TLS
#             - LDAP_URL
#             - LDAP_USE_TLS
#             - MAX_PARTICIPANTS
#             - PROSODY_RESERVATION_ENABLED
#             - PROSODY_RESERVATION_REST_BASE_URL
#             - PROSODY_ENABLE_RATE_LIMITS
#             - PROSODY_RATE_LIMIT_LOGIN_RATE
#             - PROSODY_RATE_LIMIT_SESSION_RATE
#             - PROSODY_RATE_LIMIT_TIMEOUT
#             - PROSODY_RATE_LIMIT_ALLOW_RANGES
#             - PROSODY_RATE_LIMIT_CACHE_SIZE
#             - PUBLIC_URL
#             - TURN_CREDENTIALS
#             - TURN_HOST
#             - TURNS_HOST
#             - TURN_PORT
#             - TURNS_PORT
#             - TURN_TRANSPORT
#             - TZ
#             - XMPP_DOMAIN
#             - XMPP_AUTH_DOMAIN
#             - XMPP_GUEST_DOMAIN
#             - XMPP_MUC_DOMAIN
#             - XMPP_INTERNAL_MUC_DOMAIN
#             - XMPP_MODULES
#             - XMPP_MUC_MODULES
#             - XMPP_MUC_CONFIGURATION
#             - XMPP_INTERNAL_MUC_MODULES
#             - XMPP_RECORDER_DOMAIN
#             - XMPP_PORT
#         networks:
#             meet.jitsi:
#                 aliases:
#                     - ${XMPP_SERVER:-xmpp.meet.jitsi}
# 
#     # Focus component
#     jicofo:
#         image: jitsi/jicofo:${JITSI_IMAGE_VERSION:-unstable}
#         restart: ${RESTART_POLICY:-unless-stopped}
#         ports:
#             - '127.0.0.1:8888:8888'
#         volumes:
#             - ${CONFIG}/jicofo:/config:Z
#         environment:
#             - AUTH_TYPE
#             - BRIDGE_AVG_PARTICIPANT_STRESS
#             - BRIDGE_STRESS_THRESHOLD
#             - ENABLE_AUTH
#             - ENABLE_AUTO_OWNER
#             - ENABLE_CODEC_VP8
#             - ENABLE_CODEC_VP9
#             - ENABLE_CODEC_H264
#             - ENABLE_CODEC_OPUS_RED
#             - ENABLE_JVB_XMPP_SERVER
#             - ENABLE_OCTO
#             - ENABLE_RECORDING
#             - ENABLE_SCTP
#             - ENABLE_AUTO_LOGIN
#             - JICOFO_AUTH_PASSWORD
#             - JICOFO_BRIDGE_REGION_GROUPS
#             - JICOFO_ENABLE_BRIDGE_HEALTH_CHECKS
#             - JICOFO_CONF_INITIAL_PARTICIPANT_WAIT_TIMEOUT
#             - JICOFO_CONF_SINGLE_PARTICIPANT_TIMEOUT
#             - JICOFO_CONF_SOURCE_SIGNALING_DELAYS
#             - JICOFO_CONF_MAX_AUDIO_SENDERS
#             - JICOFO_CONF_MAX_VIDEO_SENDERS
#             - JICOFO_CONF_STRIP_SIMULCAST
#             - JICOFO_CONF_SSRC_REWRITING
#             - JICOFO_ENABLE_HEALTH_CHECKS
#             - JICOFO_ENABLE_REST
#             - JICOFO_HEALTH_CHECKS_USE_PRESENCE
#             - JICOFO_MULTI_STREAM_BACKWARD_COMPAT
#             - JICOFO_OCTO_REGION
#             - JIBRI_BREWERY_MUC
#             - JIBRI_REQUEST_RETRIES
#             - JIBRI_PENDING_TIMEOUT
#             - JIGASI_BREWERY_MUC
#             - JIGASI_SIP_URI
#             - JVB_BREWERY_MUC
#             - JVB_XMPP_AUTH_DOMAIN
#             - JVB_XMPP_INTERNAL_MUC_DOMAIN
#             - JVB_XMPP_PORT
#             - JVB_XMPP_SERVER
#             - MAX_BRIDGE_PARTICIPANTS
#             - OCTO_BRIDGE_SELECTION_STRATEGY
#             - SENTRY_DSN="${JICOFO_SENTRY_DSN:-0}"
#             - SENTRY_ENVIRONMENT
#             - SENTRY_RELEASE
#             - TZ
#             - XMPP_DOMAIN
#             - XMPP_AUTH_DOMAIN
#             - XMPP_INTERNAL_MUC_DOMAIN
#             - XMPP_MUC_DOMAIN
#             - XMPP_RECORDER_DOMAIN
#             - XMPP_SERVER
#             - XMPP_PORT
#         depends_on:
#             - prosody
#         networks:
#             meet.jitsi:
# 
#     # Video bridge
#     jvb:
#         image: jitsi/jvb:${JITSI_IMAGE_VERSION:-unstable}
#         restart: ${RESTART_POLICY:-unless-stopped}
#         ports:
#             - '${JVB_PORT:-10000}:${JVB_PORT:-10000}/udp'
#             - '127.0.0.1:${JVB_COLIBRI_PORT:-8080}:8080'
#         volumes:
#             - ${CONFIG}/jvb:/config:Z
#         environment:
#             - DOCKER_HOST_ADDRESS
#             - ENABLE_COLIBRI_WEBSOCKET
#             - ENABLE_JVB_XMPP_SERVER
#             - ENABLE_OCTO
#             - JVB_ADVERTISE_IPS
#             - JVB_ADVERTISE_PRIVATE_CANDIDATES
#             - JVB_AUTH_USER
#             - JVB_AUTH_PASSWORD
#             - JVB_BREWERY_MUC
#             - JVB_DISABLE_STUN
#             - JVB_PORT
#             - JVB_MUC_NICKNAME
#             - JVB_STUN_SERVERS
#             - JVB_OCTO_BIND_ADDRESS
#             - JVB_OCTO_REGION
#             - JVB_OCTO_RELAY_ID
#             - JVB_WS_DOMAIN
#             - JVB_WS_SERVER_ID
#             - JVB_XMPP_AUTH_DOMAIN
#             - JVB_XMPP_INTERNAL_MUC_DOMAIN
#             - JVB_XMPP_PORT
#             - JVB_XMPP_SERVER
#             - PUBLIC_URL
#             - SENTRY_DSN="${JVB_SENTRY_DSN:-0}"
#             - SENTRY_ENVIRONMENT
#             - SENTRY_RELEASE
#             - COLIBRI_REST_ENABLED
#             - SHUTDOWN_REST_ENABLED
#             - TZ
#             - XMPP_AUTH_DOMAIN
#             - XMPP_INTERNAL_MUC_DOMAIN
#             - XMPP_SERVER
#             - XMPP_PORT
#         depends_on:
#             - prosody
#         networks:
#             meet.jitsi:
# 
# # Custom network so all services can communicate using a FQDN
# networks:
#     meet.jitsi:

FROM jitsi/web:unstable as jitsi-web
FROM jitsi/prosody:unstable as jitsi-prosody
FROM jitsi/jicofo:unstable as jitsi-jicofo
FROM jitsi/jvb:unstable as jitsi-jvb

ENV JVB_AUTH_USER=admin
ENV JVB_AUTH_PASSWORD=admin

LABEL maintainer="AmneziaVPN"
