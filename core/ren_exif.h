#pragma once
// Minimal EXIF reader (JPEG APP1 / bare TIFF): IFD0 + Exif IFD, common tags only.
// Replaces dEXIF; values are formatted simply (ASCII as-is, rationals as decimals,
// ExposureTime as "1/250"), which is close to but not identical with dEXIF's decoding.
#include <QMap>
#include <QString>

namespace ren {

// Tag name -> value. Returns false if the file has no EXIF block.
bool readExif(const QString& path, QMap<QString, QString>& tags);

// True if `name` is a tag this reader knows, even when the current file lacks it.
bool isKnownExifTag(const QString& name);

}  // namespace ren
