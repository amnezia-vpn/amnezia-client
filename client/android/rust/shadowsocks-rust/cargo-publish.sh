#!/bin/bash -e

set -x

ROOT_DIR=$(dirname $0)
cd ${ROOT_DIR:?}

package_ordered="crates/shadowsocks crates/shadowsocks-service ."

## dry-run
cargo check

for p in ${package_ordered:?}; do
    cargo update -p shadowsocks
    cargo update -p shadowsocks-service
    echo "====> dry-run publish $p"
    cargo publish --verbose --locked --dry-run --manifest-path "${p:?}/Cargo.toml"
    echo "====> publishing $p"
    cargo publish --verbose --locked --manifest-path "${p:?}/Cargo.toml"

    # this seems to be enough time to let crates.io update
    sleep 10
done
