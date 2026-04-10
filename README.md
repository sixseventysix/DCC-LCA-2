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

## Cloud Deployment: Location Map Viewer

**Live:** https://location-map.samarthkulkarni.workers.dev

A serverless web application deployed on **Cloudflare Workers** that geocodes an arbitrary location string and renders it on an interactive map in the browser.

### What it does

- User enters any location (e.g. "Eiffel Tower" or "Paris")
- The app queries the Nominatim geocoding API and returns up to 10 matching results (if output is fuzzy)
- All results are plotted as markers on a Leaflet map
- A "Did you mean" list lets the user select the intended location; clicking one focuses the map on that marker and dismisses the others

### Deployment

The application is deployed as a **Cloudflare Worker**; a serverless function that runs at the edge on Cloudflare's global network. There is no origin server; every request is handled directly at a data centre close to the user.

Deployment is done via the Wrangler CLI:

```bash
npm run deploy   # runs: wrangler deploy
```

The entry point is `src/worker.js`, configured in `wrangler.toml`.

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
