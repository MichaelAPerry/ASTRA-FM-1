# ASTRA FM-1

An offline astronomical music instrument for the M-VAVE FM-1. Working title; original project on branch `astra`.
MIKU-FM1 is an unmodified infrastructure reference, not the destination.

## First engineering milestone

- Shared fixed-point C solar-longitude calculation, compiled for host, PI32 firmware and browser WebAssembly.
- Real `.fwsc` running in the actual [fm1-emu](https://github.com/joakimeriksson/fm1-emu), not a simulated instrument UI.
- Temporary diagnostic signal responds to the calculation; PLAY starts/stops it and SELECT changes the fixture date.
- Static companion site with the real emulator, local date exploration, build provenance and offline asset caching.

This is **not a completed astrological instrument**. Planets, Moon, natal profiles, houses, interpretations,
offline time entry/synchronization and the ambient music engine remain future work. No hardware test or live
Pages deployment is claimed. Physical installation is deliberately not offered; the package identity
`FM-1_ASTRADEV` is provisional and has not been checked for updater compatibility or uniqueness.

The owner selected **slowly evolving ambient textures** as the musical direction to explore next.
Specific voices, harmony, planet mappings and final visual identity await collaborative decisions.

## Develop

In the prepared cloud environment, `source /workspace/tooling/activate.sh`.
Use the existing isolated checkout; do not create a worktree unless explicitly requested.
Work on `astra`; keep MIKU unchanged.

```sh
python3 -m pip install -r requirements-dev.txt
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --parallel 4
ctest --test-dir build/host --output-on-failure
gcc -std=c99 -fPIC -shared shared/astro.c -o build/libastra.so
python3 tools/compare_sun.py
python3 tools/build_native.py
python3 tools/emu_smoke.py
python3 tools/build_site.py
```

Native build needs `JIELI_TOOLCHAIN` and `AC79_SDK`; exact versions/checksums are checked before packaging.
See [setup](docs/SETUP.md), [architecture and research](docs/ARCHITECTURE.md), [validation](docs/VALIDATION.md),
and [licensing](THIRD_PARTY.md). The CI workflow repeats builds and emulator tests and uploads the full site.
Pushes to main deploy the validated artifact to GitHub Pages. The astra branch builds and tests only;
merge it to main to publish. Manual deployment is also available on main. Pages must use GitHub Actions
as its source; the raw repository root is not the generated website.

For local browser development, serve `build/site` with `python3 -m http.server --directory build/site 8000`.
Do not open it as a file: WebAssembly and service workers require HTTP/HTTPS. After its cache is ready,
the site, calculation and emulator assets work with the network disabled. The browser calculator and emulated
device have separate date controls at this milestone.
