use cfg_if::cfg_if;

cfg_if! {
    if #[cfg(any(target_os = "macos",
                 target_os = "ios",
                 target_os = "freebsd",
                 target_os = "netbsd",
                 target_os = "openbsd"))] {
        pub mod bsd_pf;
    }
}
