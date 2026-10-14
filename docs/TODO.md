# TODO

## Ideas (not for 3.0.0)

- More sidecar types (for example RawTherapee `.pp3`): add the extension to `sidecarExtensions()` in `core/ren_exif.cpp`.
- Read `.dop` data? Not planned: it is a DxO-only format.
- XMP properties not mapped yet: the `exif:` attributes of a sidecar (date, GPS), `crs:` (Camera Raw) settings.
- IPTC from TIFF files (only JPEG is read); TIFF-embedded XMP is found by content already.
