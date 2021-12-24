//! Manager API errors

use std::io;

use thiserror::Error;

use super::protocol::Error as ProtocolError;

/// Manager Error
#[derive(Error, Debug)]
pub enum Error {
    #[error("{0}")]
    IoError(#[from] io::Error),
    #[error(transparent)]
    ProtocolError(#[from] ProtocolError),
}

impl From<Error> for io::Error {
    fn from(e: Error) -> io::Error {
        match e {
            Error::IoError(e) => e,
            Error::ProtocolError(e) => From::from(e),
        }
    }
}
