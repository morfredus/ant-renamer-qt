#pragma once
// Minimal EXIF reader (JPEG APP1 / bare TIFF): IFD0 + Exif IFD, common tags only.
// Replaces dEXIF; values are formatted simply (ASCII as-is, rationals as decimals,
// ExposureTime as "1/250"), which is close to but not identical with dEXIF's decoding.
#include <QMap>
#include <QString>
#include <QStringList>

namespace ren {

// Tag name -> value (EXIF, IPTC and XMP; the XMP sidecar of the file, if any, is read first and wins).
// Returns false if nothing was found.
bool readExif(const QString& path, QMap<QString, QString>& tags);

// Extensions of the sidecar files that follow a picture when it is renamed: XMP (Lightroom, Capture One,
// digiKam, darktable...) and DxO PhotoLab's .dop.
const QStringList& sidecarExtensions();

// The sidecar files a picture can have for these extensions, existing or not: IMG.ext (Lightroom...) and
// IMG.CR2.ext (digiKam...), also with an upper-case extension, always in that order.
// Empty for a file that is itself a sidecar. Same order for any path, so two lists (old and new name) can be
// paired by index.
QStringList sidecarCandidates(const QString& path, const QStringList& exts);

// True if `name` is a tag this reader knows, even when the current file lacks it.
bool isKnownExifTag(const QString& name);

}  // namespace ren
