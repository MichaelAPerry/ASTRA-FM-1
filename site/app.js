const output = document.querySelector('#position');
try {
  const response = await fetch('astro.wasm');
  if (!response.ok) throw new Error('calculation engine unavailable');
  const {instance} = await WebAssembly.instantiate(await response.arrayBuffer());
  const calculate = event => {
    event?.preventDefault();
    const raw = document.querySelector('#utc').value;
    const instant = new Date(raw + 'Z');
    if (!Number.isFinite(instant.getTime()) || instant.getUTCFullYear() < 1950 || instant.getUTCFullYear() > 2050) {
      output.textContent = 'Enter a UTC date between 1950 and 2050.'; return;
    }
    const seconds = Math.floor((instant.getTime()-Date.UTC(2000,0,1))/1000);
    const day = Math.floor(seconds/86400);
    const lon = instance.exports.astra_sun(day, seconds-day*86400);
    if (lon < 0) throw new Error('date outside the calculation interval');
    output.textContent = `Sun ${(lon/1000).toFixed(3)}° · ${raw.replace('T',' ')} UTC`;
  };
  document.querySelector('#sky').addEventListener('submit', calculate);
  calculate();
  const meta = await (await fetch('manifest.json')).json();
  document.querySelector('#build').textContent = `Development build ${meta.commit.slice(0,12)} · SHA-256 ${meta.package_sha256}`;
} catch(error) { output.textContent = `Unable to calculate: ${error.message}`; }
if ('serviceWorker' in navigator) {
  try {
    await navigator.serviceWorker.register('sw.js');
    navigator.serviceWorker.addEventListener('message', event => {
      if (event.data === 'offline-ready') document.querySelector('#offline').textContent = 'All instrument assets are cached for offline use.';
    });
    await navigator.serviceWorker.ready;
    const cache = await caches.keys();
    if (cache.some(x => x.startsWith('astra-'))) document.querySelector('#offline').textContent = 'Offline cache available.';
  } catch(error) { document.querySelector('#offline').textContent = `Offline caching unavailable: ${error.message}`; }
}
