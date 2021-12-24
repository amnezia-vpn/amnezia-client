use futures;
use std::io;

pub async fn create_signal_monitor() -> io::Result<()> {
    // FIXME: What can I do ...
    // Blocks forever
    futures::empty::<(), io::Error>().await
}
