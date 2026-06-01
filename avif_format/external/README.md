# AVIF dependency source

The `cpf_avif` Visual Studio project expects libavif at `external/libavif` and builds it statically before linking the plugin.

The pre-build step clones the pinned libavif release automatically if `external/libavif/CMakeLists.txt` is missing:

```powershell
git clone --branch v1.4.2 --depth 1 https://github.com/AOMediaCodec/libavif.git external/libavif
```

Build output goes to `external/build/libavif/<Configuration>` and is intentionally ignored by git.
