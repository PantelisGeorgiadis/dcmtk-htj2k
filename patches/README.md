# Patches for DCMTK-HTJ2K Builds

Patches applied at build time to third-party sources. They are intended to be applied on top of the exact versions used by the build scripts.

## dcmtk-ofwhere-emscripten.patch

**Target:** DCMTK `ofstd/libsrc/ofwhere.c`  
**Used by:** `build_wasm.bat` (WebAssembly / Emscripten build)

**Purpose:** DCMTK’s `ofwhere.c` implements `OFgetExecutablePath()` per platform. Emscripten/WASM has no real executable path. Without this patch, the Emscripten build would hit the `#else` branch and fail with “unsupported platform”.

**Change:** Adds an `#elif defined(__EMSCRIPTEN__)` branch that returns a placeholder path (`"."`) and a zero `dirname_length`, so the rest of DCMTK can build and run under Emscripten.

**Apply manually (if needed):**

```bash
cd ots_wasm/dcmtk
git apply /path/to/dcmtk-htj2k/patches/dcmtk-ofwhere-emscripten.patch
```

If you use a different DCMTK version, the patch may need to be updated to match the current `ofwhere.c` (e.g. line numbers or surrounding `#if` blocks).
