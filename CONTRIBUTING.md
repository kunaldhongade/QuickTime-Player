# Contributing

Build and test changes with the debug preset before submitting them:

```bash
./scripts/build.sh
./scripts/test.sh
```

Format C++ with the repository `.clang-format`. Keep playback and indexing logic in C++, keep QML
focused on presentation, and do not expose raw mpv handles to QML.

Changes to stepping or timestamp resolution should include deterministic CFR, fractional-rate,
VFR, B-frame, boundary, and stale-index coverage where applicable. Do not commit generated media
when `scripts/generate_test_media.sh` can reproduce it.

Do not publish binary packages until the licensing gate described in `README.md` and
`THIRD_PARTY_NOTICES.md` is complete.
