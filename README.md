# DCC Continuous Assessment
Samarth Kulkarni

1032222418

Roll No. 50, Batch C2


## Tasks
1. Bully Election Algorithm
2. Network File System (NFS)
3. Cloud Deployment

## Submission Rules
- Push all files to GitHub
- Include README + output
- Be ready for live demo

## Deadline
10/04/2026

## Bully Election Algorithm

A single-file C simulation of the Bully Election Algorithm, implemented in `bully-election/bully.c`.

### What it does

The Bully Algorithm is a leader election algorithm for distributed systems. Each process has a unique numeric ID. The process with the highest ID among the currently alive processes becomes the coordinator. When any process notices the coordinator is gone, it triggers an election:

1. It sends an `ELECTION` message to every process with a higher ID.
2. If a higher-ID process is alive, it replies `OK` and takes over the election by repeating step 1 with its own ID.
3. If no higher process replies, the initiator declares itself coordinator and broadcasts a `COORDINATOR` message to all other processes.

The algorithm is inherently synchronous — each step blocks waiting for a response before the next one begins. This is assumed by the protocol: silence within a bounded time means the process is dead.

### Scenarios simulated

1. **Initial election** — no coordinator exists; the lowest process triggers one and P5 wins.
2. **Coordinator crash** — P5 (coordinator) fails; P2 detects it and starts a new election; P4 wins.
3. **Cascade failure** — P4 and P3 also fail; P1 detects it; P2 wins.
4. **Recovery** — P5 recovers and immediately starts an election, bullying its way back to the top.
5. **Last survivor** — all processes except P1 fail; P1 wins with no competition.

### How to run

```bash
cd bully-election
gcc -Wall -o bully bully.c && ./bully
```

### Implementation note

The simulation runs in a single process. Each "process" is a struct with an ID and an `UP`/`DOWN` status. Message passing is modelled as direct function calls — sending an `ELECTION` message to P4 is just checking whether P4's status is `UP`. The recursive call stack mirrors the election chain: P1 calls `elect(1)`, which hands off to `elect(2)`, which hands off to `elect(5)`, which wins.

### Why not pthreads or separate Unix processes?

Both approaches were attempted. The core difficulty is that the Bully Algorithm is synchronous by design — a process sends `ELECTION` and waits for an `OK` before deciding what to do next. With pthreads, all higher-ID threads receive the `ELECTION` message concurrently and all start their own elections simultaneously, flooding the system with redundant elections before anyone can declare victory. There is no clean synchronisation point without effectively serialising the threads back into the sequential model, at which point threads add complexity with no benefit. Separate OS processes over TCP have the same problem, compounded by stale messages from previous elections sitting in socket buffers and being picked up by the next election's receive call. Election algorithms designed for truly asynchronous systems (e.g. Ring Election, Raft leader election) handle this by including a term/epoch number in every message so stale messages are discarded. The Bully Algorithm has no such mechanism — it was designed for synchronous systems and is best simulated as one.

## Network File System (NFS)

An NFS setup across two Linux VMs demonstrating a server-client shared filesystem over a real network.

### What it does

- An NFS server exports `/exports/shared` using `nfs-kernel-server`
- An NFS client mounts that directory at `/mnt/shared` over NFS
- Files written on either side are immediately visible on the other

Both nodes are Ubuntu VMs running under [OrbStack](https://orbstack.dev) on Apple Silicon, connected via OrbStack's internal network.

### Structure

```
nfs/
├── doc.md        # full technical documentation
└── output/
    └── output.txt    # terminal session demonstrating the setup
```

See [nfs/doc.md](nfs/doc.md) for setup steps, export configuration, and an explanation of how NFS works.

## Cloud Deployment: Location Map Viewer

**Live:** https://location-map.samarthkulkarni.workers.dev

A serverless web application deployed on **Cloudflare Workers** that geocodes an arbitrary location string and renders it on an interactive map in the browser.

### What it does

- User enters any location (e.g. "Eiffel Tower" or "Paris")
- The app queries the Nominatim geocoding API and returns up to 10 matching results (if output is fuzzy)
- All results are plotted as markers on a Leaflet map
- A "Did you mean" list lets the user select the intended location; clicking one focuses the map on that marker and dismisses the others

### Deployment

**Cloudflare Workers** is a serverless compute platform. Unlike a traditional deployment where you rent a virtual machine or a server, provision it, install dependencies, and keep it running 24/7, Workers lets you deploy a single JavaScript file. Cloudflare takes that file and distributes it across its global network of data centres. When a user makes a request, it is handled by the data centre closest to them; there is no single origin server.

This model is called *edge computing*: the code runs at the "edge" of the network, close to the user, rather than in one centralised location. The application has no idle cost; you are only charged for actual requests.

**Wrangler** is the official command-line tool for Cloudflare Workers. It handles authentication with your Cloudflare account, bundles your JavaScript, and uploads it to the platform. The project is configured via `wrangler.toml`, which tells Wrangler the name of the Worker, the entry point file, and the compatibility settings.

To deploy:

```bash
wrangler login        # opens a browser to authenticate with your Cloudflare account
npm run deploy        # runs: wrangler deploy — bundles src/worker.js and pushes it live
```

Once deployed, Cloudflare assigns a public URL of the form `https://<worker-name>.<your-subdomain>.workers.dev`. This application is live at https://location-map.samarthkulkarni.workers.dev.

### How rendering works

Everything is rendered client-side in the browser. The Worker serves a single HTML page (inlined as a template string in `worker.js`) which includes:

- Leaflet CSS/JS loaded from a CDN
- Inline JavaScript that calls the Worker's own `/map` API endpoint on form submit
- The Worker proxies the Nominatim request server-side (to avoid CORS issues and to set the required `User-Agent` header), then returns JSON to the browser
- The browser JS receives the coordinates and uses Leaflet to render an interactive OSM tile map with markers; no page reload required

### APIs used

**Nominatim** (`nominatim.openstreetmap.org`)
- Free geocoding API provided by the OpenStreetMap community
- Accepts a free-form location string and returns ranked matches with latitude, longitude, and a display name
- Used with `limit=10` to return multiple candidates
- Requires a `User-Agent` header identifying the application (set server-side in the Worker)

**OpenStreetMap Tiles** (`tile.openstreetmap.org`)
- Raster map tiles served in the standard `/{z}/{x}/{y}.png` slippy map format
- Consumed directly by Leaflet to render the base map

**Leaflet** (`leafletjs.com`)
- Open-source JavaScript library for interactive maps
- Handles tile loading, map panning/zooming, and marker placement in the browser
- Used to place a marker for each geocoding result and fit the map bounds to show all of them
