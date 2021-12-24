//! Daemonize server process

use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(unix)] {
        mod unix;
        pub use self::unix::daemonize;
    } else {
        compile_error!("Process daemonization is not supported by the current platform");
    }
}
