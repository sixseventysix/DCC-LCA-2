NFS_SERVER_IP = 192.168.100.10
MOUNT_POINT   = /mnt/nfs

.PHONY: server client test clean help

help:
	@echo "Targets:"
	@echo "  make server   - build and start the NFS server"
	@echo "  make client   - start the NFS client and mount the share (opens a shell)"
	@echo "  make test     - run automated read/write verification"
	@echo "  make clean    - stop and remove all containers, volumes, networks"

## Build the server image and start it
server:
	docker compose build nfs-server
	docker compose up -d nfs-server
	@echo "Waiting for NFS server to be ready..."
	@for i in $$(seq 1 20); do \
		if docker logs nfs-server 2>&1 | grep -qiE "NFS SERVER UP|dispatcher started"; then \
			echo "[OK] NFS server is up at $(NFS_SERVER_IP)"; \
			docker logs nfs-server 2>&1 | grep -i "export\|path" | head -5; \
			break; \
		fi; \
		sleep 2; \
		if [ "$$i" -eq 20 ]; then \
			echo "[ERR] Server did not start in time. Logs:"; \
			docker logs nfs-server; \
			exit 1; \
		fi; \
	done

## Start the client container and mount the NFS share, then open an interactive shell
client:
	docker compose build nfs-client
	docker compose up -d nfs-client
	@echo "Mounting $(NFS_SERVER_IP):/exports → $(MOUNT_POINT) ..."
	@docker exec nfs-client bash -c \
		"mkdir -p $(MOUNT_POINT) && mountpoint -q $(MOUNT_POINT) || \
		 mount -t nfs -o vers=4 $(NFS_SERVER_IP):/exports $(MOUNT_POINT)"
	@echo "[OK] Share mounted at $(MOUNT_POINT)"
	@echo "Entering client shell — shared directory is at $(MOUNT_POINT)"
	@echo "Type 'exit' to leave."
	docker exec -it nfs-client bash

## Automated verification: write from client, read from server, and vice versa
test:
	@echo "--- Writing from client ---"
	@TIMESTAMP=$$(date '+%Y-%m-%d %H:%M:%S'); \
	docker exec nfs-client bash -c \
		"echo 'Hello from NFS client — $$TIMESTAMP' > $(MOUNT_POINT)/client_test.txt"
	@echo "--- Reading from server ---"
	@docker exec nfs-server cat /exports/client_test.txt
	@echo "--- Writing from server ---"
	@TIMESTAMP=$$(date '+%Y-%m-%d %H:%M:%S'); \
	docker exec nfs-server bash -c \
		"echo 'Hello from NFS server — $$TIMESTAMP' > /exports/server_test.txt"
	@echo "--- Reading from client ---"
	@docker exec nfs-client cat $(MOUNT_POINT)/server_test.txt
	@echo "--- Directory listing (client view) ---"
	@docker exec nfs-client ls -la $(MOUNT_POINT)/
	@echo "[OK] NFS read/write test passed."

## Stop and remove everything
clean:
	docker compose down -v --remove-orphans
