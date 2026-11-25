#ifndef QTIMAGEVIEWER_IMAGEFILES_H
#define QTIMAGEVIEWER_IMAGEFILES_H

#include <QString>
#include <QStringList>

namespace imgview {

/*! The outcome of expanding the command line paths into a list of image files. */
struct ImageScan
{
  //! Files to show, in command line order, without duplicates.
  QStringList files;
  //! Human readable problems, in the order they were found.
  QStringList warnings;
};

//! Whether \a path ends in a file name suffix Qt can read an image from.
bool hasImageSuffix(const QString &path);

//! Every file name suffix (without the dot) Qt can read, lower case and sorted.
QStringList supportedImageSuffixes();

/*!
 * Expands \a paths into the list of files to show.
 *
 * A file argument is taken as it is: an argument the user names explicitly is
 * shown, or reported as unreadable by the viewer if it turns out not to be an
 * image. A directory argument contributes the image files it contains, sorted by
 * name, and with \a recursive also those of its subdirectories. Hidden entries
 * are ignored, as are files that can be reached through more than one argument.
 */
ImageScan collectImages(const QStringList &paths, bool recursive);

}// namespace imgview

#endif
