#!/bin/bash
set -e

mkdir -p /exports && chmod 777 /exports
mkdir -p /var/run/ganesha

# Start dbus (required by ganesha)
dbus-daemon --system --fork || true
sleep 1

# Start rpcbind
rpcbind || true
sleep 1

echo "===================================="
echo "  NFS SERVER UP"
echo "===================================="

# Start ganesha in foreground
exec /usr/bin/ganesha.nfsd -F -L /dev/stdout -f /etc/ganesha/ganesha.conf
