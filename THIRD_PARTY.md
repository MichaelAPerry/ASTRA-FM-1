# Licenses and source provenance

ASTRA's authored code is GPL-3.0-only, compatible with this repository's GPLv3 license and the selected platform.

| Component | Exact source | License / use |
| --- | --- | --- |
| Dinghy platform and packager | zvenson/dinghy `f77bf03283bbb2c05294b01cdad68a30b410f93d` | GPL-3.0-only; fetched into ignored build dependencies, staged with its LICENSE, original file headers retained |
| Platform ancestry | Felucca / SLOOP / sloopDX / OberVave as attributed in Dinghy files | Upstream notices remain in the staged source; ship corresponding source with any binary distribution |
| FM-1 emulator | joakimeriksson/fm1-emu `9de1ef053a3c4bf56f16f05b5f0de193f4608a1d` | MIT; LICENSE copied beside the web emulator |
| JieLi SDK package assets | AC79NN_SDK_V1.2.1_2023-12-13, `d179b4484759423312073f5fbb232501aa491047` | SDK Apache-2.0 notices; required files retain pinned upstream SHA-256 verification; no SDK networking libraries linked |
| Astronomy Engine | Don Cross, astronomy-engine 2.1.19 | MIT; independent test oracle only, not linked into firmware/browser |
| Solar approximation | USNO [Approximate Solar Coordinates](https://aa.usno.navy.mil/faq/sun_approx) | Formula independently implemented using integer turns; no copied library source |
| Fixed-point sine technique | Dinghy `firmware/src/dinghy.c` | Refined parabolic sine adapted under GPL-3.0-only; copyright and source attribution here and in the code |
| MIKU-FM1 | MichaelAPerry/MIKU-FM1 | Read-only reference for packaging, smoke gates, Pages/release provenance; no artwork or application code copied |
| Fonts in firmware | Dinghy's generated assets and their source notices | Preserve upstream notices; see dependency tree |

Compiler, Zig, Pillow, Rust and browser-test tooling are development tools, not site runtime dependencies.
No proprietary ephemeris library, remote horoscope service or embedded API key is used.

Before any public binary release, include a matching source archive containing ASTRA, the exact staged
platform source, upstream license notices and instructions for the SDK/compiler inputs. CI prepares that
archive. Development firmware is not an endorsed or hardware-tested release.
