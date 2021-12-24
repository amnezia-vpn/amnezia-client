//! HTTP Client

use hyper::{Body, Client};

use super::connector::{BypassConnector, ProxyConnector};

pub type ProxyHttpClient = Client<ProxyConnector, Body>;
pub type BypassHttpClient = Client<BypassConnector, Body>;
