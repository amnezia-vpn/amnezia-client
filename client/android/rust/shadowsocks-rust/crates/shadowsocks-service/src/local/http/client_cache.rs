//! Cached HTTP client for remote server

use std::{sync::Arc, time::Duration};

use hyper::{Body, Client};
use lru_time_cache::LruCache;
use shadowsocks::config::ServerAddr;
use tokio::sync::Mutex;

use crate::local::{context::ServiceContext, loadbalancing::ServerIdent};

use super::{connector::ProxyConnector, http_client::ProxyHttpClient};

/// Cached HTTP client for remote servers
pub struct ProxyClientCache {
    context: Arc<ServiceContext>,
    cache: Mutex<LruCache<ServerAddr, ProxyHttpClient>>,
}

impl ProxyClientCache {
    pub fn new(context: Arc<ServiceContext>) -> ProxyClientCache {
        ProxyClientCache {
            context,
            cache: Mutex::new(LruCache::with_expiry_duration_and_capacity(Duration::from_secs(60), 5)),
        }
    }

    pub async fn get_connected(&self, server: &Arc<ServerIdent>) -> ProxyHttpClient {
        let server_config = server.server_config();

        let mut cache = self.cache.lock().await;
        if let Some(client) = cache.get(server_config.addr()) {
            return client.clone();
        }

        // Create a new client
        let client = Client::builder().build::<_, Body>(ProxyConnector::new(self.context.clone(), server.clone()));
        cache.insert(server_config.addr().clone(), client.clone());

        client
    }
}
