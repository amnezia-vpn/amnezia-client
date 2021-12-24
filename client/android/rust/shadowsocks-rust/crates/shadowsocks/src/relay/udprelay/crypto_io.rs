//! Crypto protocol for ShadowSocks UDP
//!
//! Payload with stream cipher
//! ```plain
//! +-------+----------+
//! |  IV   | Payload  |
//! +-------+----------+
//! | Fixed | Variable |
//! +-------+----------+
//! ```
//!
//! Payload with AEAD cipher
//!
//! ```plain
//! UDP (after encryption, *ciphertext*)
//! +--------+-----------+-----------+
//! | NONCE  |  *Data*   |  Data_TAG |
//! +--------+-----------+-----------+
//! | Fixed  | Variable  |   Fixed   |
//! +--------+-----------+-----------+
//! ```
use std::io::{self, Cursor, ErrorKind};

use byte_string::ByteStr;
use bytes::{BufMut, BytesMut};
use log::{debug, trace};

use crate::{
    context::Context,
    crypto::v1::{random_iv_or_salt, Cipher, CipherCategory, CipherKind},
    relay::socks5::Address,
};

/// Encrypt payload into ShadowSocks UDP encrypted packet
pub fn encrypt_payload(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    addr: &Address,
    payload: &[u8],
    dst: &mut BytesMut,
) {
    match method.category() {
        CipherCategory::None => {
            dst.reserve(addr.serialized_len() + payload.len());
            addr.write_to_buf(dst);
            dst.put_slice(payload);
        }
        #[cfg(feature = "stream-cipher")]
        CipherCategory::Stream => encrypt_payload_stream(context, method, key, addr, payload, dst),
        CipherCategory::Aead => encrypt_payload_aead(context, method, key, addr, payload, dst),
    }
}

#[cfg(feature = "stream-cipher")]
fn encrypt_payload_stream(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    addr: &Address,
    payload: &[u8],
    dst: &mut BytesMut,
) {
    let iv_len = method.iv_len();
    let addr_len = addr.serialized_len();

    // Packet = IV + ADDRESS + PAYLOAD
    dst.reserve(iv_len + addr_len + payload.len());

    // Generate IV
    dst.resize(iv_len, 0);
    let iv = &mut dst[..iv_len];

    if iv_len > 0 {
        loop {
            random_iv_or_salt(iv);
            if !context.check_nonce_and_set(iv) {
                break;
            }
        }

        trace!("UDP packet generated stream iv {:?}", ByteStr::new(iv));
    } else {
        context.check_nonce_and_set(iv);
    }

    let mut cipher = Cipher::new(method, key, &iv);

    addr.write_to_buf(dst);
    dst.put_slice(payload);
    let m = &mut dst[iv_len..];
    cipher.encrypt_packet(m);
}

fn encrypt_payload_aead(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    addr: &Address,
    payload: &[u8],
    dst: &mut BytesMut,
) {
    let salt_len = method.salt_len();
    let addr_len = addr.serialized_len();

    // Packet = IV + ADDRESS + PAYLOAD + TAG
    dst.reserve(salt_len + addr_len + payload.len() + method.tag_len());

    // Generate IV
    dst.resize(salt_len, 0);
    let salt = &mut dst[..salt_len];

    if salt_len > 0 {
        loop {
            random_iv_or_salt(salt);
            if !context.check_nonce_and_set(salt) {
                break;
            }
        }

        trace!("UDP packet generated aead salt {:?}", ByteStr::new(salt));
    } else {
        context.check_nonce_and_set(salt);
    }

    let mut cipher = Cipher::new(method, key, salt);

    addr.write_to_buf(dst);
    dst.put_slice(payload);

    unsafe {
        dst.advance_mut(method.tag_len());
    }

    let m = &mut dst[salt_len..];
    cipher.encrypt_packet(m);
}

/// Decrypt payload from ShadowSocks UDP encrypted packet
pub async fn decrypt_payload(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    payload: &mut [u8],
) -> io::Result<(usize, Address)> {
    match method.category() {
        CipherCategory::None => {
            let mut cur = Cursor::new(payload);
            match Address::read_from(&mut cur).await {
                Ok(address) => {
                    let pos = cur.position() as usize;
                    let payload = cur.into_inner();
                    payload.copy_within(pos.., 0);
                    Ok((payload.len() - pos, address))
                }
                Err(..) => {
                    let err = io::Error::new(ErrorKind::InvalidData, "parse udp packet Address failed");
                    Err(err)
                }
            }
        }
        #[cfg(feature = "stream-cipher")]
        CipherCategory::Stream => decrypt_payload_stream(context, method, key, payload).await,
        CipherCategory::Aead => decrypt_payload_aead(context, method, key, payload).await,
    }
}

#[cfg(feature = "stream-cipher")]
async fn decrypt_payload_stream(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    payload: &mut [u8],
) -> io::Result<(usize, Address)> {
    let plen = payload.len();
    let iv_len = method.iv_len();

    if plen < iv_len {
        let err = io::Error::new(ErrorKind::InvalidData, "udp packet too short for iv");
        return Err(err);
    }

    let (iv, data) = payload.split_at_mut(iv_len);
    if context.check_nonce_and_set(iv) {
        debug!("detected repeated iv {:?}", ByteStr::new(iv));
        return Err(io::Error::new(io::ErrorKind::Other, "detected repeated iv"));
    }

    trace!("UDP packet got stream IV {:?}", ByteStr::new(iv));
    let mut cipher = Cipher::new(method, key, iv);

    assert_eq!(cipher.decrypt_packet(data), true);

    let (dn, addr) = parse_packet(data).await?;

    let data_start_idx = iv_len + dn;
    let data_length = payload.len() - data_start_idx;
    payload.copy_within(data_start_idx.., 0);

    Ok((data_length, addr))
}

async fn decrypt_payload_aead(
    context: &Context,
    method: CipherKind,
    key: &[u8],
    payload: &mut [u8],
) -> io::Result<(usize, Address)> {
    let plen = payload.len();
    let salt_len = method.salt_len();
    if plen < salt_len {
        let err = io::Error::new(ErrorKind::InvalidData, "udp packet too short for salt");
        return Err(err);
    }

    let (salt, data) = payload.split_at_mut(salt_len);
    if context.check_nonce_and_set(salt) {
        debug!("detected repeated salt {:?}", ByteStr::new(salt));
        return Err(io::Error::new(io::ErrorKind::Other, "detected repeated salt"));
    }

    trace!("UDP packet got AEAD salt {:?}", ByteStr::new(salt));

    let mut cipher = Cipher::new(method, &key, &salt);
    let tag_len = cipher.tag_len();

    if data.len() < tag_len {
        return Err(io::Error::new(io::ErrorKind::Other, "udp packet too short for tag"));
    }

    if !cipher.decrypt_packet(data) {
        return Err(io::Error::new(io::ErrorKind::Other, "invalid tag-in"));
    }

    // Truncate TAG
    let data_len = data.len() - tag_len;
    let data = &mut data[..data_len];

    let (dn, addr) = parse_packet(data).await?;

    let data_length = data_len - dn;
    let data_start_idx = salt_len + dn;
    let data_end_idx = data_start_idx + data_length;

    payload.copy_within(data_start_idx..data_end_idx, 0);

    Ok((data_length, addr))
}

async fn parse_packet(buf: &[u8]) -> io::Result<(usize, Address)> {
    let mut cur = Cursor::new(buf);
    match Address::read_from(&mut cur).await {
        Ok(address) => {
            let pos = cur.position() as usize;
            Ok((pos, address))
        }
        Err(..) => {
            let err = io::Error::new(ErrorKind::InvalidData, "parse udp packet Address failed");
            Err(err)
        }
    }
}
