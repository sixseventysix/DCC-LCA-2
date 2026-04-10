const HTML_FORM = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Location Map Viewer</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
  <style>
    body { font-family: 'Inter', sans-serif; }
    #map { width: 700px; height: 450px; margin-top: 12px; }
    #resultList a { cursor: pointer; text-decoration: underline; color: blue; }
    #resultList li { margin-bottom: 4px; }
  </style>
</head>
<body>
  <h1>Location Map Viewer</h1>
  <h3>Simple application that takes in an arbitrary location and outputs a marker on a map</h3>
  <p><small>Powered by <a href="https://nominatim.openstreetmap.org" target="_blank">Nominatim</a>, <a href="https://www.openstreetmap.org" target="_blank">OpenStreetMap</a>, and <a href="https://leafletjs.com" target="_blank">Leaflet</a>.</small></p>

  <form id="mapForm">
    <label for="locationInput">Location:</label>
    <input type="text" id="locationInput" name="location"
      placeholder="Input any location" autocomplete="off" required />
    <button type="submit">Show Map</button>
  </form>
  <hr>
  <div id="status"></div>
  <div id="didyoumean"></div>
  <div id="map" style="display:none"></div>

  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <script>
    const form = document.getElementById('mapForm');
    const status = document.getElementById('status');
    const didyoumean = document.getElementById('didyoumean');
    const mapEl = document.getElementById('map');
    let map = null;
    let markers = [];

    function initMap(lat, lon) {
      mapEl.style.display = 'block';
      if (map) { map.remove(); map = null; }
      markers = [];
      map = L.map('map').setView([lat, lon], 5);
      L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '© OpenStreetMap contributors',
      }).addTo(map);
      return map;
    }

    function focusResult(results, idx) {
      const r = results[idx];

      // hide all markers, show only the selected one
      markers.forEach((m, i) => {
        if (i === idx) m.addTo(map).openPopup();
        else m.remove();
      });

      map.setView([r.lat, r.lon], 13);
      status.innerHTML =
        '<p><strong>' + r.displayName + '</strong></p>' +
        '<p><small>Lat: ' + r.lat + ' &nbsp;|&nbsp; Lon: ' + r.lon + '</small></p>';
    }

    form.addEventListener('submit', async (e) => {
      e.preventDefault();
      const location = document.getElementById('locationInput').value.trim();
      if (!location) return;

      status.innerHTML = '<p><em>Looking up location...</em></p>';
      didyoumean.innerHTML = '';
      mapEl.style.display = 'none';

      try {
        const res = await fetch('/map?location=' + encodeURIComponent(location));
        const data = await res.json();

        if (!res.ok) {
          status.innerHTML = '<p><strong>Error:</strong> ' + data.error + '</p>';
          return;
        }

        const results = data.results;
        const m = initMap(results[0].lat, results[0].lon);

        const bounds = results.map((r, i) => {
          const marker = L.marker([r.lat, r.lon]).addTo(m)
            .bindPopup('<strong>' + r.displayName + '</strong>');
          markers.push(marker);
          return [r.lat, r.lon];
        });

        if (results.length === 1) {
          m.setView([results[0].lat, results[0].lon], 13);
          markers[0].openPopup();
          status.innerHTML =
            '<p><strong>' + results[0].displayName + '</strong></p>' +
            '<p><small>Lat: ' + results[0].lat + ' &nbsp;|&nbsp; Lon: ' + results[0].lon + '</small></p>';
        } else {
          m.fitBounds(bounds, { padding: [30, 30] });
          status.innerHTML = '<p>Found ' + results.length + ' results. Did you mean:</p>';
          let html = '<ol id="resultList">';
          results.forEach((r, i) => {
            html += '<li><a id="result-' + i + '" data-index="' + i + '">' + r.displayName + '</a></li>';
          });
          html += '</ol>';
          didyoumean.innerHTML = html;

          // clicking a marker highlights the corresponding link
          markers.forEach((marker, i) => {
            marker.on('click', () => {
              document.querySelectorAll('#resultList a').forEach(a => a.style.fontWeight = '');
              const link = document.getElementById('result-' + i);
              if (link) link.style.fontWeight = 'bold';
            });
          });

          document.getElementById('resultList').addEventListener('click', (e) => {
            const a = e.target.closest('a');
            if (!a) return;
            const idx = parseInt(a.dataset.index);
            focusResult(results, idx);
            didyoumean.innerHTML = '';
          });
        }
      } catch (err) {
        status.innerHTML = '<p><strong>Error:</strong> Something went wrong. Please try again.</p>';
      }
    });
  </script>
</body>
</html>`;

async function geocode(location) {
  const url =
    'https://nominatim.openstreetmap.org/search?format=json&limit=10&q=' +
    encodeURIComponent(location);

  const res = await fetch(url, {
    headers: {
      'User-Agent': 'location-map-viewer/1.0 (cloudflare-worker)',
      'Accept-Language': 'en',
    },
  });

  if (!res.ok) throw new Error('Nominatim request failed');

  const raw = await res.json();
  if (!raw.length) return null;

  return raw.map(({ lat, lon, display_name }) => ({
    lat: parseFloat(lat),
    lon: parseFloat(lon),
    displayName: display_name,
  }));
}

export default {
  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === '/') {
      return new Response(HTML_FORM, {
        headers: { 'Content-Type': 'text/html;charset=UTF-8' },
      });
    }

    if (url.pathname === '/map') {
      const location = url.searchParams.get('location');

      if (!location) {
        return Response.json({ error: 'Missing ?location= parameter' }, { status: 400 });
      }

      let results;
      try {
        results = await geocode(location);
      } catch {
        return Response.json({ error: 'Failed to contact geocoding service' }, { status: 502 });
      }

      if (!results) {
        return Response.json({ error: 'Location not found: ' + location }, { status: 404 });
      }

      return Response.json({ results });
    }

    return new Response('Not Found', { status: 404 });
  },
};
