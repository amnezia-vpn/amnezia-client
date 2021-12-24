//! Manager server listener

use std::io;

use log::warn;

use crate::{config::ManagerAddr, context::Context, relay::udprelay::MAXIMUM_UDP_PAYLOAD_SIZE};

use super::{
    datagram::{ManagerDatagram, ManagerSocketAddr},
    error::Error,
    protocol::{ManagerProtocol, ManagerRequest},
};

/// Manager server Listener
pub struct ManagerListener {
    socket: ManagerDatagram,
}

impl ManagerListener {
    /// Create a `ManagerDatagram` binding to requested `bind_addr`
    pub async fn bind(context: &Context, bind_addr: &ManagerAddr) -> io::Result<ManagerListener> {
        ManagerDatagram::bind(context, bind_addr)
            .await
            .map(|socket| ManagerListener { socket })
    }

    pub async fn recv_from(&mut self) -> Result<(ManagerRequest, ManagerSocketAddr), Error> {
        let mut buf = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        let (n, peer_addr) = self.socket.recv_from(&mut buf).await?;
        Ok((ManagerRequest::from_bytes(&buf[..n])?, peer_addr))
    }

    pub async fn send_to<P: ManagerProtocol>(&mut self, data: &P, target: &ManagerSocketAddr) -> Result<(), Error> {
        let buf = data.to_bytes()?;
        let n = self.socket.send_to(&buf, target).await?;
        if n != buf.len() {
            warn!("manager send_to {} bytes != buffer {} bytes", n, buf.len());
        }
        Ok(())
    }

    pub fn local_addr(&self) -> io::Result<ManagerSocketAddr> {
        self.socket.local_addr()
    }
}
