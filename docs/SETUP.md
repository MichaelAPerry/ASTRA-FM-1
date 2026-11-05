# Reproducible tools

Tested on Linux x86-64, GCC 14.2, Python 3.12, Rust 1.99.1 (including wasm32-unknown-unknown), Node 24.
CI uses Ubuntu 24.04. Python dependencies are pinned in requirements-dev.txt.

The cloud environment retains the compiler and SDK. In a fresh machine:

```sh
mkdir -p build/toolchain
curl -fL --retry 3 https://pkgman.jieliapp.com/s/linux-toolchain -o build/toolchain/compiler.tar.xz
echo 'a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd  build/toolchain/compiler.tar.xz' | sha256sum --check
tar -xJf build/toolchain/compiler.tar.xz -C build/toolchain
export JIELI_TOOLCHAIN="$PWD/build/toolchain/jieli-linux-toolchains-20260730.1"
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git build/ac79-sdk
test "$(git -C build/ac79-sdk rev-parse HEAD)" = d179b4484759423312073f5fbb232501aa491047
export AC79_SDK="$PWD/build/ac79-sdk"
```

The vendor endpoint can change: if the checksum differs, investigate the official release instead of updating
the expected checksum merely to pass. Keep TLS verification enabled. The SDK's three required file hashes
are also enforced by tools/build_native.py.

Native build fetches pinned Dinghy into build/deps and stages it in build/platform. It never resets, cleans
or writes the reference repository. To use the cloud dependency caches set
`DINGHY_SOURCE=/workspace/tooling/dinghy` and `FM1_EMU_SOURCE=/workspace/tooling/fm1-emu`.
Emulator source for host validation must be at the same revision as the website:

```sh
git clone https://github.com/joakimeriksson/fm1-emu build/deps/fm1-emu
git -C build/deps/fm1-emu checkout --detach 9de1ef053a3c4bf56f16f05b5f0de193f4608a1d
cargo build --manifest-path build/deps/fm1-emu/Cargo.toml --locked --release -p fm1-emu -j4
export FM1_EMU_BIN="$PWD/build/deps/fm1-emu/target/release/fm1-emu"
rustup target add wasm32-unknown-unknown
```

No live services are needed for host or batch firmware validation. Restart the static HTTP server for browser
development in each new session; processes do not survive snapshots.
