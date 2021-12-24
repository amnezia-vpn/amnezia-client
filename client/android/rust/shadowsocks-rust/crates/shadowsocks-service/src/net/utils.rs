//! Network Utilities

use std::io;

use tokio::io::{AsyncRead, AsyncReadExt};

/// Consumes all data from `reader` and throws away until EOF
pub async fn ignore_until_end<R>(reader: &mut R) -> io::Result<()>
where
    R: AsyncRead + Unpin,
{
    let mut buffer = [0u8; 2048];

    loop {
        let n = reader.read(&mut buffer).await?;
        if n == 0 {
            break;
        }
    }

    Ok(())
}
