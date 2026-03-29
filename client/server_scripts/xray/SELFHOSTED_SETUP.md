# Self-Hosted XRay Setup

This folder now contains a one-shot bootstrap script for deploying `amnezia-xray`
on a fresh VPS in a layout that the FBLink backend can auto-discover over SSH.

## What it does

- builds the `amnezia-xray` docker image from the local `Dockerfile`
- generates and persists:
  - `server.json`
  - `xray_uuid.key`
  - `xray_short_id.key`
  - `xray_public.key`
  - `xray_private.key`
- recreates the docker container with `--restart always`
- publishes the chosen TCP port
- opens the local host firewall when possible
- prints the final connection parameters and verification commands

## Files expected by backend

The backend VIP XRay auto-discovery reads these files over SSH:

- `/opt/amnezia/xray/server.json`
- `/opt/amnezia/xray/xray_uuid.key`
- `/opt/amnezia/xray/xray_short_id.key`
- `/opt/amnezia/xray/xray_public.key`
- `/opt/amnezia/xray/xray_private.key`

## Recommended defaults

- container: `amnezia-xray`
- config dir: `/opt/amnezia/xray`
- port: `8443`
- SNI: `www.googletagmanager.com`

## Quick start on a new VPS

Copy the `xray` folder to the server and run:

```bash
chmod +x ./install_selfhosted.sh
./install_selfhosted.sh --port 8443 --sni www.googletagmanager.com
```

If you want a fixed public address in the printed summary:

```bash
./install_selfhosted.sh \
  --port 8443 \
  --sni www.googletagmanager.com \
  --public-host 138.124.101.69
```

## Verification

On the VPS:

```bash
docker ps --format 'table {{.Names}}\t{{.Ports}}\t{{.Status}}'
docker exec amnezia-xray sh -lc 'nc -z 127.0.0.1 8443 && echo XRay is listening'
```

From Windows:

```powershell
Test-NetConnection <server-ip> -Port 8443
```

Expected:

- `docker ps` shows `0.0.0.0:8443->8443/tcp`
- `nc -z` succeeds inside the container
- `TcpTestSucceeded : True` from Windows

## Important notes

- If external TCP reachability is still false, open the port in the provider firewall.
- Re-running the script keeps existing UUID/keys by default.
- Use `--force-regenerate` only when you intentionally want to rotate credentials.
- Use `--rebuild-image` when you want to rebuild the docker image from the current repo files.

## Backend integration

After the server is up:

1. add or update the server record in FBLink admin/backend
2. ensure SSH host/user/password are correct
3. make sure the server host/IP in backend matches the public address
4. refresh client config or re-login

The backend will then auto-read the XRay runtime files from the server.
