//! Manager client

use log::warn;

use crate::{config::ManagerAddr, context::Context, net::ConnectOpts, relay::udprelay::MAXIMUM_UDP_PAYLOAD_SIZE};

use super::{
    datagram::ManagerDatagram,
    error::Error,
    protocol::{
        AddRequest,
        AddResponse,
        ListRequest,
        ListResponse,
        ManagerProtocol,
        PingRequest,
        PingResponse,
        RemoveRequest,
        RemoveResponse,
        StatRequest,
    },
};

/// Client for communicating with Manager
pub struct ManagerClient {
    socket: ManagerDatagram,
}

macro_rules! impl_command {
    ($command:ident, $req:ty, $rsp:ty) => {
        /// Send command
        pub async fn $command(&mut self, req: &$req) -> Result<$rsp, Error> {
            self.request(req).await
        }
    };
}

impl ManagerClient {
    impl_command!(add, AddRequest, AddResponse);

    impl_command!(list, ListRequest, ListResponse);

    impl_command!(ping, PingRequest, PingResponse);

    impl_command!(remote, RemoveRequest, RemoveResponse);

    /// Create a `ManagerDatagram` for sending data to manager
    pub async fn connect(
        context: &Context,
        bind_addr: &ManagerAddr,
        connect_opts: &ConnectOpts,
    ) -> Result<ManagerClient, Error> {
        ManagerDatagram::connect(context, bind_addr, connect_opts)
            .await
            .map(|socket| ManagerClient { socket })
            .map_err(Into::into)
    }

    /// Send `stat` report
    pub async fn stat(&mut self, req: &StatRequest) -> Result<(), Error> {
        let buf = req.to_bytes()?;
        let n = self.socket.send(&buf).await?;
        if n != buf.len() {
            warn!("manager send {} bytes != buffer {} bytes", n, buf.len());
        }
        Ok(())
    }

    async fn request<S, R>(&mut self, req: &S) -> Result<R, Error>
    where
        S: ManagerProtocol,
        R: ManagerProtocol,
    {
        let buf = req.to_bytes()?;
        let n = self.socket.send(&buf).await?;
        if n != buf.len() {
            warn!("manager send {} bytes != buffer {} bytes", n, buf.len());
        }

        let mut buf = [0u8; MAXIMUM_UDP_PAYLOAD_SIZE];
        let n = self.socket.recv(&mut buf).await?;
        R::from_bytes(&buf[..n]).map_err(Into::into)
    }
}
