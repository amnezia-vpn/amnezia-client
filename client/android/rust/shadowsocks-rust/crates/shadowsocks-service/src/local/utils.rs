//! Shadowsocks Local Utilities

use std::{io, net::SocketAddr, time::Duration};

use futures::future::{self, Either};
use log::trace;
use shadowsocks::{
    config::ServerConfig,
    relay::{
        socks5::Address,
        tcprelay::utils::{copy_from_encrypted, copy_to_encrypted},
    },
};
use tokio::{
    io::{copy, AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt},
    time,
};

use crate::local::net::AutoProxyIo;

pub async fn establish_tcp_tunnel<PR, PW, SR, SW>(
    svr_cfg: &ServerConfig,
    plain_reader: &mut PR,
    plain_writer: &mut PW,
    shadow_reader: &mut SR,
    shadow_writer: &mut SW,
    peer_addr: SocketAddr,
    target_addr: &Address,
) -> io::Result<()>
where
    PR: AsyncRead + Unpin,
    PW: AsyncWrite + Unpin,
    SR: AsyncRead + AutoProxyIo + Unpin,
    SW: AsyncWrite + AutoProxyIo + Unpin,
{
    if shadow_reader.is_proxied() && shadow_writer.is_proxied() {
        trace!(
            "established tcp tunnel {} <-> {} through sever {} (outbound: {})",
            peer_addr,
            target_addr,
            svr_cfg.external_addr(),
            svr_cfg.addr(),
        );
    } else {
        trace!("established tcp tunnel {} <-> {} bypassed", peer_addr, target_addr);
        return establish_tcp_tunnel_bypassed(
            plain_reader,
            plain_writer,
            shadow_reader,
            shadow_writer,
            peer_addr,
            target_addr,
        )
        .await;
    }

    // https://github.com/shadowsocks/shadowsocks-rust/issues/232
    //
    // Protocols like FTP, clients will wait for servers to send Welcome Message without sending anything.
    //
    // Wait at most 500ms, and then sends handshake packet to remote servers.
    {
        let mut buffer = [0u8; 8192];
        match time::timeout(Duration::from_millis(500), plain_reader.read(&mut buffer)).await {
            Ok(Ok(0)) => {
                // EOF. Just terminate right here.
                return Ok(());
            }
            Ok(Ok(n)) => {
                // Send the first packet.
                shadow_writer.write_all(&buffer[..n]).await?;
            }
            Ok(Err(err)) => return Err(err),
            Err(..) => {
                // Timeout. Send handshake to server.
                shadow_writer.write(&[]).await?;

                trace!(
                    "tcp tunnel {} -> {} sent handshake without data",
                    peer_addr,
                    target_addr
                );
            }
        }
    }

    let l2r = copy_to_encrypted(svr_cfg.method(), plain_reader, shadow_writer);
    let r2l = copy_from_encrypted(svr_cfg.method(), shadow_reader, plain_writer);

    tokio::pin!(l2r);
    tokio::pin!(r2l);

    match future::select(l2r, r2l).await {
        Either::Left((Ok(..), ..)) => {
            trace!("tcp tunnel {} -> {} closed", peer_addr, target_addr);
        }
        Either::Left((Err(err), ..)) => {
            trace!("tcp tunnel {} -> {} closed with error: {}", peer_addr, target_addr, err);
        }
        Either::Right((Ok(..), ..)) => {
            trace!("tcp tunnel {} <- {} closed", peer_addr, target_addr);
        }
        Either::Right((Err(err), ..)) => {
            trace!("tcp tunnel {} <- {} closed with error: {}", peer_addr, target_addr, err);
        }
    }

    Ok(())
}

async fn establish_tcp_tunnel_bypassed<PR, PW, SR, SW>(
    plain_reader: &mut PR,
    plain_writer: &mut PW,
    shadow_reader: &mut SR,
    shadow_writer: &mut SW,
    peer_addr: SocketAddr,
    target_addr: &Address,
) -> io::Result<()>
where
    PR: AsyncRead + Unpin,
    PW: AsyncWrite + Unpin,
    SR: AsyncRead + Unpin,
    SW: AsyncWrite + Unpin,
{
    let l2r = copy(plain_reader, shadow_writer);
    let r2l = copy(shadow_reader, plain_writer);

    tokio::pin!(l2r);
    tokio::pin!(r2l);

    match future::select(l2r, r2l).await {
        Either::Left((Ok(..), ..)) => {
            trace!("tcp tunnel {} -> {} closed", peer_addr, target_addr);
        }
        Either::Left((Err(err), ..)) => {
            trace!("tcp tunnel {} -> {} closed with error: {}", peer_addr, target_addr, err);
        }
        Either::Right((Ok(..), ..)) => {
            trace!("tcp tunnel {} <- {} closed", peer_addr, target_addr);
        }
        Either::Right((Err(err), ..)) => {
            trace!("tcp tunnel {} <- {} closed with error: {}", peer_addr, target_addr, err);
        }
    }

    Ok(())
}
