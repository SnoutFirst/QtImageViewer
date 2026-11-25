#include "playlist.h"

#include <algorithm>
#include <numeric>
#include <utility>

namespace imgview {

Playlist::Playlist(QStringList entries, PlaylistOrder order, QRandomGenerator *random)
  : m_entries(std::move(entries)), m_order(order), m_random(random == nullptr ? QRandomGenerator::global() : random)
{
  m_cycle.resize(static_cast<std::size_t>(size()));
  startCycle(true, -1);
}

bool Playlist::isEmpty() const { return m_entries.isEmpty(); }

int Playlist::size() const
{
  // QStringList::size() is an int on Qt 5 and a qsizetype on Qt 6; going through
  // a qsizetype keeps this a real conversion (rather than a useless cast) on both.
  const qsizetype count = m_entries.size();
  return static_cast<int>(count);
}

PlaylistOrder Playlist::order() const { return m_order; }

const QStringList &Playlist::entries() const { return m_entries; }

QString Playlist::current() const
{
  const int entry = currentEntry();
  return entry < 0 ? QString() : m_entries.at(entry);
}

int Playlist::position() const { return isEmpty() ? 0 : m_position + 1; }

QString Playlist::advance()
{
  if (isEmpty()) { return {}; }

  if (m_position + 1 < size()) {
    ++m_position;
  } else {
    const int lastShown = currentEntry();
    startCycle(true, lastShown);
  }
  return current();
}

QString Playlist::retreat()
{
  if (isEmpty()) { return {}; }

  if (m_position > 0) {
    --m_position;
  } else {
    const int lastShown = currentEntry();
    startCycle(false, lastShown);
  }
  return current();
}

QString Playlist::restart()
{
  if (isEmpty()) { return {}; }

  if (m_order == PlaylistOrder::Shuffled) {
    const int shown = currentEntry();
    startCycle(true, shown);
  } else {
    // Nothing to draw: the first entry is the one a sequential playlist starts with.
    m_position = 0;
  }
  return current();
}

int Playlist::currentEntry() const { return isEmpty() ? -1 : m_cycle.at(static_cast<std::size_t>(m_position)); }

/*!
 * Fills \a m_cycle with the next permutation to walk, and puts the cursor at its
 * first (forwards) or last (backwards) entry.
 *
 * \a boundary is the entry that must not sit at that end, so that looping does
 * not show the same image twice in a row; pass -1 for "no restriction".
 */
void Playlist::startCycle(bool forwards, int boundary)
{
  const int count = size();

  std::iota(m_cycle.begin(), m_cycle.end(), 0);

  if (m_order == PlaylistOrder::Shuffled) {
    std::shuffle(m_cycle.begin(), m_cycle.end(), *m_random);

    if (count > 1) {
      const int edge = forwards ? 0 : count - 1;
      if (m_cycle.at(static_cast<std::size_t>(edge)) == boundary) {
        // Swap the offending entry with any other one: swapping two entries of
        // a permutation leaves a permutation.
        const auto offset = (*m_random)() % static_cast<quint32>(count - 1);
        const int other = forwards ? static_cast<int>(offset) + 1 : static_cast<int>(offset);
        std::swap(m_cycle.at(static_cast<std::size_t>(edge)), m_cycle.at(static_cast<std::size_t>(other)));
      }
    }
  }

  m_position = forwards ? 0 : count - 1;
  if (m_position < 0) { m_position = 0; }
}

}// namespace imgview
