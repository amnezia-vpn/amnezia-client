XRAY_DIR=/opt/amnezia/xray

# /opt/amnezia/xray is a docker volume, so these files outlive the container.
# Regenerating any of them revokes every config already handed out, so each one is
# created only when it is really missing.
#
# "Really missing" is checked by content, not by file size. A failed generation used
# to leave a file holding a single newline, which is one byte, so a size test would
# call it present and the key would never be repaired. With the volume that state is
# permanent: nothing short of manual SSH would clear it.
has_key() {
    [ -f "$1" ] && [ -n "$(tr -d '[:space:]' < "$1" 2>/dev/null)" ]
}

if ! has_key $XRAY_DIR/xray_uuid.key; then
    XRAY_CLIENT_ID=$(xray uuid) && [ -n "$XRAY_CLIENT_ID" ] && printf '%s\n' "$XRAY_CLIENT_ID" > $XRAY_DIR/xray_uuid.key
fi

if ! has_key $XRAY_DIR/xray_short_id.key; then
    XRAY_SHORT_ID=$(openssl rand -hex 8) && [ -n "$XRAY_SHORT_ID" ] && printf '%s\n' "$XRAY_SHORT_ID" > $XRAY_DIR/xray_short_id.key
fi

# The private key is the one that must never change: it is what every issued config
# is bound to. If only the public half is missing it gets derived from the private
# one rather than rotating the pair.
if has_key $XRAY_DIR/xray_private.key && ! has_key $XRAY_DIR/xray_public.key; then
    XRAY_PRIVATE_KEY=$(tr -d '[:space:]' < $XRAY_DIR/xray_private.key)
    DERIVED=$(xray x25519 -i "$XRAY_PRIVATE_KEY" 2>/dev/null)
    XRAY_PUBLIC_KEY=$(printf '%s\n' "$DERIVED" | sed -n 's/.*(PublicKey):[[:space:]]*//p' | head -1)
    [ -z "$XRAY_PUBLIC_KEY" ] && XRAY_PUBLIC_KEY=$(printf '%s\n' "$DERIVED" | sed -n 's/.*[Pp]ublic[ ]*[Kk]ey:[[:space:]]*//p' | head -1)
    XRAY_PUBLIC_KEY=$(echo $XRAY_PUBLIC_KEY | tr -d ' ')
    if [ -n "$XRAY_PUBLIC_KEY" ]; then
        printf '%s\n' "$XRAY_PUBLIC_KEY" > $XRAY_DIR/xray_public.key
    fi
fi

if ! has_key $XRAY_DIR/xray_private.key || ! has_key $XRAY_DIR/xray_public.key; then
    KEYPAIR=$(xray x25519)
    XRAY_PRIVATE_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*[Pp]rivate[ ]*[Kk]ey:[[:space:]]*//p' | head -1)
    XRAY_PUBLIC_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*(PublicKey):[[:space:]]*//p' | head -1)
    [ -z "$XRAY_PUBLIC_KEY" ] && XRAY_PUBLIC_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*[Pp]ublic[ ]*[Kk]ey:[[:space:]]*//p' | head -1)

    XRAY_PRIVATE_KEY=$(echo $XRAY_PRIVATE_KEY | tr -d ' ')
    XRAY_PUBLIC_KEY=$(echo $XRAY_PUBLIC_KEY | tr -d ' ')

    # Both or neither. A half-written pair hands clients a public key the server is
    # not using, and it would look complete enough never to be repaired.
    if [ -n "$XRAY_PRIVATE_KEY" ] && [ -n "$XRAY_PUBLIC_KEY" ]; then
        printf '%s\n' "$XRAY_PRIVATE_KEY" > $XRAY_DIR/xray_private.key.tmp
        printf '%s\n' "$XRAY_PUBLIC_KEY" > $XRAY_DIR/xray_public.key.tmp
        mv $XRAY_DIR/xray_private.key.tmp $XRAY_DIR/xray_private.key
        mv $XRAY_DIR/xray_public.key.tmp $XRAY_DIR/xray_public.key
    else
        rm -f $XRAY_DIR/xray_private.key.tmp $XRAY_DIR/xray_public.key.tmp
        echo "amnezia_xray_keygen=failed"
    fi
fi
