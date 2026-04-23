# amnezia-cli

`amnezia-cli` is a headless command-line client built on top of the same core models, controllers, and VPN runtime used by the desktop application.

## Build

From the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build --target amnezia-cli -j4
```

The binary is produced at:

```bash
./build/client/cli/amnezia-cli
```

## Command Model

- `connect` and `disconnect` talk to a local CLI daemon over `QLocalSocket`.
- `status` queries the daemon when it is running; otherwise it returns a local disconnected snapshot.
- The daemon keeps VPN state and allows the CLI process itself to stay short-lived.
- Management commands such as `servers`, `countries`, `containers`, `install`, and `logs cleanup` can run directly or through the daemon.

## Common Commands

```bash
./build/client/cli/amnezia-cli status
./build/client/cli/amnezia-cli daemon start
./build/client/cli/amnezia-cli servers list
./build/client/cli/amnezia-cli countries list --index 0
./build/client/cli/amnezia-cli containers list --index 0
./build/client/cli/amnezia-cli connect --index 0
./build/client/cli/amnezia-cli disconnect
```

JSON output is available for automation on any command:

```bash
./build/client/cli/amnezia-cli status --json
```

## Supported Operations

- Inspect VPN state: `status`
- Control VPN connection: `connect`, `disconnect`
- Control the daemon: `daemon start|stop|status`
- Manage saved servers: `servers list|show|add|import|remove|set-default|scan`
- Manage available countries for API-backed configs: `countries list|set`
- Manage containers: `containers list|set-default|remove`
- Install a new server or container: `install server`, `install container`
- Cleanup local logs: `logs cleanup`

## Notes

- The daemon redirects its own stdout and stderr to avoid breaking `--json` output from the foreground CLI process.
