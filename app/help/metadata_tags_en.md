# EXIF, IPTC and XMP tags

The [Use EXIF info](actions_en.md#exif) action reads the information stored inside the picture files and lets you use it in the mask, like any other `%tag%` (see ["Formatting"](format_en.md)). Three kinds of metadata are read:

- **EXIF**: what the camera writes (date, camera, exposure...). Read from JPEG and TIFF-based files (including most RAW files such as DNG, NEF, CR2, ARW).
- **IPTC**: what is typed in a photo manager (keywords, caption, author, place...). Tags start with `Iptc`. Read from JPEG files.
- **XMP**: the modern equivalent of IPTC, written by Lightroom, Capture One, Photoshop, digiKam... Tags start with `Xmp`. Read from any file that embeds an XMP packet (JPEG, PNG, WebP, TIFF, DNG...) and from `.xmp` files.

Tag names are not case-sensitive. A tag that the file does not contain gives an empty text; the action reports "No EXIF info found" only when the file has none of the three kinds of metadata.

## Good to know

- **Same information in IPTC and XMP.** Lightroom and most photo managers write both; others write only XMP. Use `$if` to take the first one that exists, e.g. `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`.
- **Several values** (keywords, authors) are joined with a comma and a space. Use `$if` to avoid a leading separator when a tag is empty, e.g. `%DateTimeOriginal%$if(%XmpKeywords%, - %XmpKeywords%,)%ext%`.
- **Dates.** EXIF, IPTC and XMP dates are all given as `yyyy:MM:dd HH:mm:ss`. The characters that cannot be used in a file name (such as ":") are replaced by "-", so `2024:05:17 14:30:05` gives `2024-05-17 14-30-05`. The "Offset" setting of the action shifts every date tag that has this layout.
- **Codes.** Tags such as `Flash`, `Orientation` or `MeteringMode` give the number stored in the file (see the EXIF specification for the meaning of each value).
- **Forbidden characters.** The values are cleaned like every file name: `/ \ : * ? " < > |` are replaced.
- **XMP sidecar files.** The `.xmp` file next to a picture (`IMG.xmp` or `IMG.CR2.xmp`) is read with the picture and wins over the XMP embedded in it: the keywords, title and rating you gave to a RAW file in your photo manager are usable. The sidecar (and a DxO `.dop` file) is also renamed with the picture (Options, Processing).
- **Limits.** Only the first 512 KB of each file are examined. IPTC is read from JPEG only.

## EXIF tags {#exif}

| Tag | Meaning |
|---|---|
| `%DateTimeOriginal%` | Date and time the picture was taken |
| `%DateTimeDigitized%` | Date and time the picture was digitized |
| `%DateTime%` | Date and time of the last change of the image, written by the software that edited it (not the date of the file on the disk) |
| `%Make%` | Camera manufacturer |
| `%Model%` | Camera model |
| `%BodySerialNumber%` | Camera serial number |
| `%LensMake%` | Lens manufacturer |
| `%LensModel%` | Lens model |
| `%ExposureTime%` | Shutter speed, as 1/250 for fast speeds |
| `%FNumber%` | Aperture (f-number), e.g. 2.8 |
| `%ISOSpeedRatings%` | ISO sensitivity |
| `%FocalLength%` | Focal length in mm |
| `%FocalLengthIn35mmFilm%` | Focal length in mm, 35 mm equivalent |
| `%ExposureBiasValue%` | Exposure compensation (EV) |
| `%ShutterSpeedValue%` | Shutter speed, APEX value |
| `%ApertureValue%` | Aperture, APEX value |
| `%ExposureProgram%` | Exposure program (code) |
| `%ExposureMode%` | Exposure mode (code) |
| `%MeteringMode%` | Metering mode (code) |
| `%Flash%` | Flash status (code; 0 = did not fire) |
| `%WhiteBalance%` | White balance (0 = auto, 1 = manual) |
| `%SceneCaptureType%` | Scene type (code) |
| `%Orientation%` | Orientation of the picture (1 to 8) |
| `%ExifImageWidth%` | Width in pixels |
| `%ExifImageLength%` | Height in pixels |
| `%XResolution%` | Horizontal resolution |
| `%YResolution%` | Vertical resolution |
| `%ResolutionUnit%` | Resolution unit (2 = inch, 3 = cm) |
| `%ColorSpace%` | Color space (1 = sRGB) |
| `%Software%` | Software that wrote the file |
| `%Artist%` | Photographer (EXIF field) |
| `%Copyright%` | Copyright (EXIF field) |
| `%ImageDescription%` | Description (EXIF field) |
| `%UserComment%` | User comment (often empty or not readable) |
| `%ExifVersion%` | EXIF version |
| `%MakerNote%` | Manufacturer notes (rarely usable) |

## IPTC tags {#iptc}

| Tag | Meaning |
|---|---|
| `%IptcKeywords%` | Keywords (several values joined with a comma) |
| `%IptcCaption%` | Caption / description |
| `%IptcHeadline%` | Headline |
| `%IptcObjectName%` | Title (object name) |
| `%IptcByline%` | Author / photographer |
| `%IptcBylineTitle%` | Author's job title |
| `%IptcCopyright%` | Copyright notice |
| `%IptcCredit%` | Credit line |
| `%IptcSource%` | Source |
| `%IptcCity%` | City |
| `%IptcSublocation%` | Location within the city |
| `%IptcState%` | State / province |
| `%IptcCountry%` | Country name |
| `%IptcCountryCode%` | Country code (e.g. FRA) |
| `%IptcDateCreated%` | Date of creation, as 2024:05:17 |
| `%IptcTimeCreated%` | Time of creation, as 14:30:05 |
| `%IptcCategory%` | Category |
| `%IptcSupplementalCategories%` | Supplemental categories |
| `%IptcSpecialInstructions%` | Special instructions |
| `%IptcUrgency%` | Urgency (1 to 8) |
| `%IptcWriter%` | Caption writer |
| `%IptcTransmissionReference%` | Job identifier (transmission reference) |

## XMP tags {#xmp}

| Tag | Meaning |
|---|---|
| `%XmpTitle%` | Title (dc:title) |
| `%XmpDescription%` | Description (dc:description) |
| `%XmpKeywords%` | Keywords (dc:subject, joined with a comma) |
| `%XmpHierarchicalKeywords%` | Lightroom keyword hierarchy, e.g. Places\|France\|Paris |
| `%XmpCreator%` | Author / photographer (dc:creator) |
| `%XmpRights%` | Copyright (dc:rights) |
| `%XmpRating%` | Rating, 0 to 5 stars (-1 = rejected) |
| `%XmpLabel%` | Color label (e.g. Red) |
| `%XmpCreateDate%` | Date and time of creation, as 2024:05:17 14:30:05 |
| `%XmpModifyDate%` | Date and time of last modification, same layout |
| `%XmpDateCreated%` | Date of creation of the content (photoshop:DateCreated) |
| `%XmpCreatorTool%` | Software that created the file |
| `%XmpHeadline%` | Headline |
| `%XmpCredit%` | Credit line |
| `%XmpSource%` | Source |
| `%XmpCity%` | City |
| `%XmpState%` | State / province |
| `%XmpCountry%` | Country name |
| `%XmpCountryCode%` | Country code |
| `%XmpLocation%` | Location (sublocation) |
| `%XmpCategory%` | Category |
| `%XmpInstructions%` | Special instructions |
| `%XmpAuthorsPosition%` | Author's job title |
| `%XmpCaptionWriter%` | Caption writer |
| `%XmpTransmissionReference%` | Job identifier |
| `%XmpLens%` | Lens description (aux:Lens) |
