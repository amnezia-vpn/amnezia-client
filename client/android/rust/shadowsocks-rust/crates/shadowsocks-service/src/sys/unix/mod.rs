use std::io;

#[allow(dead_code)]
#[cfg(not(target_os = "android"))]
pub fn set_nofile(nofile: u64) -> io::Result<()> {
    use std::io::Error;

    unsafe {
        // set both soft and hard limit
        let lim = libc::rlimit {
            rlim_cur: nofile as libc::rlim_t,
            rlim_max: nofile as libc::rlim_t,
        };

        if libc::setrlimit(libc::RLIMIT_NOFILE, &lim as *const _) < 0 {
            return Err(Error::last_os_error());
        }
    }

    Ok(())
}

#[allow(dead_code)]
#[cfg(target_os = "android")]
pub fn set_nofile(_nofile: u64) -> io::Result<()> {
    // Android doesn't have this API
    Ok(())
}
