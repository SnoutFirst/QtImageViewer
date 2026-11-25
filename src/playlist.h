#ifndef QTIMAGEVIEWER_PLAYLIST_H
#define QTIMAGEVIEWER_PLAYLIST_H

#include <QRandomGenerator>
#include <QString>
#include <QStringList>

#include <vector>

namespace imgview {

/*! The order in which a Playlist hands out its entries. */
enum class PlaylistOrder {
  //! In the order the entries were given.
  Sequential,
  //! One fresh random permutation ("cycle") after another.
  Shuffled,
};

/*!
 * The list of images to show, and the order to show them in.
 *
 * Shuffled is deliberately *not* "pick a random entry on every step": the
 * playlist walks a random permutation of its entries, so every image is shown
 * exactly once before any image is shown a second time. When a cycle is
 * exhausted a new permutation is drawn, and its first entry is guaranteed to
 * differ from the entry that was on screen, so the loop never repeats an image
 * back to back. retreat() enforces the same rule at the other end of a cycle.
 */
class Playlist
{
public:
  explicit Playlist(QStringList entries, PlaylistOrder order, QRandomGenerator *random = QRandomGenerator::global());

  bool isEmpty() const;
  int size() const;
  PlaylistOrder order() const;
  const QStringList &entries() const;

  //! The entry on screen; empty when isEmpty().
  QString current() const;
  //! 1-based position of the current entry within its cycle; 0 when isEmpty().
  int position() const;

  //! Moves to the next entry, drawing a new cycle when the current one is used up.
  QString advance();
  //! Moves to the previous entry, drawing a new cycle when the current one is used up.
  QString retreat();

  //! Draws a new cycle and shows its first entry, which is never the one shown now.
  QString restart();

private:
  void startCycle(bool forwards, int boundary);
  int currentEntry() const;

  QStringList m_entries;
  PlaylistOrder m_order;
  QRandomGenerator *m_random;
  std::vector<int> m_cycle;
  int m_position = 0;
};

}// namespace imgview

#endif
