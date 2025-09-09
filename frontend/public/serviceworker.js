const CACHE_NAME = 'bobik-pwa-v2';
const urlsToCache = [
  '/',
  '/index.html',
  '/manifest.json',
  '/icon192.png',
  '/icon512.png',
  '/icon180.png',
  '/favicon.ico',
  '/assets/attackdog.jpg',
  '/assets/dogread.jpg',
  '/assets/dogsleep.jpg',
  '/assets/dogue.jpg',
  '/assets/favicon.png',
  '/assets/garage_closed.png',
  '/assets/garage_open.png',
  '/assets/required.svg'
];

self.addEventListener('install', (event) => {
  event.waitUntil((async () => {
    const cache = await caches.open(CACHE_NAME);

    // Force network for precache (avoid HTTP cache)
    const reqs = urlsToCache.map((url) => new Request(url, { cache: 'reload' }));

    // Add individually so one failure doesn't nuke the whole install
    const results = await Promise.allSettled(reqs.map((r) => cache.add(r)));

    // Log any failures so you can fix paths/headers
    results.forEach((res, i) => {
      if (res.status === 'rejected') {
        console.error('[SW] Precache failed:', PRECACHE[i], res.reason);
      }
    });

    self.skipWaiting(); // immediately move to "waiting"
  })());
});

self.addEventListener('fetch', (event) => {
  const { request } = event;
  const url = new URL(request.url);

  // 1) Streams: never touch caches; go straight to network.
  if (
    url.pathname === '/video/' ||
    request.headers.get('accept')?.includes('multipart/x-mixed-replace') ||
    (request.destination === 'image' && url.pathname.startsWith('/video/'))
  ) {
    return;
  }

  // 2) Navigations (and auth page): always network, never cached.
  if (url.pathname === '/passwordauth') {
    event.respondWith(fetch(request, { cache: 'no-store', credentials: 'include' }));
    return;
  }

  // 3) Everything else: network-first with safe cache fallback.
  event.respondWith((async () => {
    const cache = await caches.open(CACHE_NAME);
    try {
      const netRes = await fetch(request, { cache: 'no-store', credentials: 'include' });

      // // If image failed, retry once with cache:'reload'
      // if (!netRes.ok && request.destination === 'image') {
      //   const retryRes = await fetch(request, { cache: 'reload', credentials: 'include' });
      //   if (okToCache(request, retryRes)) await cache.put(request, retryRes.clone());
      //   return retryRes;
      // }

      if (okToCache(request, netRes)) await cache.put(request, netRes.clone());
      return netRes;
    } catch (err) {
      const cached = await cache.match(request);
      if (cached) return cached;

      // last-ditch retry for images
      if (request.destination === 'image') {
        try {
          const retryRes = await fetch(request, { cache: 'reload', credentials: 'include' });
          if (okToCache(request, retryRes)) await cache.put(request, retryRes.clone());
          return retryRes;
        } catch {}
      }
      return new Response('', { status: 400 });
    }
  })());
});

// Cache only safe, same-origin, non-redirect, 200 GET responses.
function okToCache(request, response) {
  return (
    request.method === 'GET' &&
    response &&
    response.status === 200 &&
    response.type === 'basic' &&   // same-origin (not opaque)
    !response.redirected
  );
}

self.addEventListener('activate', event => {
  caches.delete(CACHE_NAME);
});