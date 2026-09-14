# WebP dependency source

The `cpf_webp` Visual Studio project expects libwebp at `external/libwebp` and builds it statically before linking the plugin.

The pre-build step clones the pinned libwebp release automatically if `external/libwebp/CMakeLists.txt` is missing:

```powershell
git clone --branch v1.5.0 --depth 1 https://github.com/webmproject/libwebp.git external/libwebp
```

Build output goes to `external/build/libwebp/<Configuration>` and is intentionally ignored by git.
