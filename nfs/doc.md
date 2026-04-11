# NFS — Technical Documentation

## Overview

This task demonstrates a working NFS (Network File System) setup across two isolated Linux virtual machines running on the same host. One VM acts as the NFS server, exporting a directory. The other acts as the NFS client, mounting that directory over the network.

Files written on the server are immediately visible on the client and vice versa — without any copying or syncing. This is the core property of NFS: a remote filesystem that behaves like a local one.

## Environment

**Host:** Apple Silicon MacBook (arm64, macOS)

**Virtualisation:** [OrbStack](https://orbstack.dev) — a lightweight VM manager for macOS that runs Linux VMs natively on Apple Silicon. Each VM gets its own IP on an internal network and can reach other VMs by IP.

**VMs:**

| Name | Role | IP |
|---|---|---|
| `nfs-server` | NFS server | 192.168.139.180 |
| `nfs-client` | NFS client | 192.168.139.77 |

Both VMs run Ubuntu (ARM64). OrbStack provides inter-VM connectivity out of the box — no manual bridge or NAT configuration required.

## Why Not Docker

The initial approach used Docker containers with NFS-Ganesha (a userspace NFS daemon). This was necessary because Docker Desktop on macOS runs containers inside a Linux VM that does not expose the kernel NFS subsystem (`/proc/fs/nfsd`) to containers, so the standard `nfs-kernel-server` package fails. Ganesha works around this but introduces complexity (dbus, rpcbind, userspace protocol stack) and UID mapping issues.

Full Linux VMs via OrbStack eliminate all of these problems. The kernel NFS subsystem is fully available, so standard `nfs-kernel-server` works without any workarounds.

## Server Setup

### Install NFS server

```bash
sudo apt update && sudo apt install -y nfs-kernel-server
```

### Create the export directory

```bash
sudo mkdir -p /exports/shared
sudo chown $USER:$USER /exports/shared
```

This directory lives entirely inside the server VM — it is not shared with the host Mac, so all reads and writes go through NFS on the client side.

### Configure the export

```bash
echo "/exports/shared 192.168.139.77(rw,sync,no_subtree_check,no_root_squash)" | sudo tee -a /etc/exports
```

Export options:

| Option | Meaning |
|---|---|
| `rw` | Client can read and write |
| `sync` | Writes are flushed to disk before the server replies |
| `no_subtree_check` | Disables subtree checking — improves reliability when the export is an entire directory |
| `no_root_squash` | Root on the client is treated as root on the server (not remapped to `nobody`) |

### Apply and start

```bash
sudo exportfs -a
sudo systemctl start nfs-kernel-server
```

Verify the export is active:

```bash
sudo exportfs -v
```

## Client Setup

### Install NFS client tools

```bash
sudo apt update && sudo apt install -y nfs-common
```

### Create mount point and mount

```bash
sudo mkdir -p /mnt/shared
sudo mount -t nfs 192.168.139.180:/exports/shared /mnt/shared
```

Verify the mount:

```bash
df -h /mnt/shared
```

## Verification

### Client → Server

Write from the client, read from the server:

```bash
# On nfs-client
echo "hello from client" > /mnt/shared/test.txt

# On nfs-server
cat /exports/shared/test.txt
```

### Server → Client

Write from the server, read from the client:

```bash
# On nfs-server
echo "hello from server" > /exports/shared/from-server.txt

# On nfs-client
cat /mnt/shared/from-server.txt
```

### UID mapping

```bash
# On nfs-server
ls -ln /exports/shared

# On nfs-client
ls -ln /mnt/shared
```

Both sides show UID 501 — the Mac user's UID, inherited by both VMs via OrbStack. Because the UIDs match, no remapping or squashing occurs and file ownership is consistent across the mount.

## How NFS Works

NFS is a distributed filesystem protocol. The server exports a directory and listens for requests. The client mounts that export, and the kernel intercepts all filesystem calls (open, read, write, stat) on that mount point and translates them into NFS RPC calls sent over the network to the server.

From the perspective of a process running on the client, `/mnt/shared` looks and behaves like a local directory. The network transport is invisible.

Key protocol details:
- **NFSv3/v4** — this setup uses whichever version the client and server negotiate (typically v4 on modern Linux)
- **RPC** — NFS uses Sun RPC (Remote Procedure Call) as its transport layer
- **Stateless (v3) vs stateful (v4)** — NFSv3 is stateless; each request is self-contained. NFSv4 is stateful and supports proper file locking
- **UID-based permissions** — NFS does not authenticate users by name; it passes raw UID/GID numbers. Both sides must agree on what those numbers mean
