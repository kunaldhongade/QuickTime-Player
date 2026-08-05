# macOS distribution

`scripts/package_macos.sh` creates a drag-to-Applications DMG. A public build requires:

1. an approved redistributable Qt/libmpv/FFmpeg dependency set;
2. completed notices and license texts under `LICENSES/`;
3. a Developer ID Application certificate; and
4. notarization and stapling after the DMG is created.

Example release build:

```bash
export FRAMEVIEWER_CODESIGN_IDENTITY="Developer ID Application: Example (TEAMID)"
export FRAMEVIEWER_FFMPEG_DIR="/path/to/approved/bin"
export FRAMEVIEWER_LICENSES_VERIFIED=YES
./scripts/package_macos.sh
xcrun notarytool submit dist/macos/FrameViewer-0.3.1-macos-arm64.dmg \
  --keychain-profile frameviewer-notary --wait
xcrun stapler staple dist/macos/FrameViewer-0.3.1-macos-arm64.dmg
spctl --assess --type open --context context:primary-signature -v \
  dist/macos/FrameViewer-0.3.1-macos-arm64.dmg
```

For local QA only, `./scripts/package_macos.sh --development` creates an ad-hoc signed DMG.
The filename identifies it as development-only and the script refuses to present it as a
distribution artifact.
