#include "catch_qt.h"

#include "playlist.h"

#include <catch2/catch_test_macros.hpp>

#include <QRandomGenerator>
#include <QSet>
#include <QString>
#include <QStringList>

namespace {

QStringList names(int count)
{
  QStringList result;
  result.reserve(count);
  for (int i = 0; i < count; ++i) { result.append(QStringLiteral("image-%1.png").arg(i)); }
  return result;
}

//! The entry on screen now, followed by the next \a count - 1 entries.
QStringList shownFromHere(imgview::Playlist &playlist, int count)
{
  QStringList shown{ playlist.current() };
  for (int i = 1; i < count; ++i) { shown.append(playlist.advance()); }
  return shown;
}

}// namespace

TEST_CASE("An empty playlist stays empty", "[playlist]")
{
  imgview::Playlist playlist({}, imgview::PlaylistOrder::Shuffled);

  REQUIRE(playlist.isEmpty());
  REQUIRE(playlist.size() == 0);
  REQUIRE(playlist.position() == 0);
  REQUIRE(playlist.current().isEmpty());
  REQUIRE(playlist.advance().isEmpty());
  REQUIRE(playlist.retreat().isEmpty());
  REQUIRE(playlist.restart().isEmpty());
}

TEST_CASE("A sequential playlist walks the entries in order and wraps around", "[playlist]")
{
  const QStringList entries = names(3);
  imgview::Playlist playlist(entries, imgview::PlaylistOrder::Sequential);

  REQUIRE(playlist.order() == imgview::PlaylistOrder::Sequential);
  REQUIRE(playlist.position() == 1);
  REQUIRE(shownFromHere(playlist, 7)
          == QStringList{
            entries.at(0), entries.at(1), entries.at(2), entries.at(0), entries.at(1), entries.at(2), entries.at(0) });
  REQUIRE(playlist.position() == 1);
}

TEST_CASE("A sequential playlist steps backwards and wraps around", "[playlist]")
{
  const QStringList entries = names(3);
  imgview::Playlist playlist(entries, imgview::PlaylistOrder::Sequential);

  // Stepping back from the first entry lands on the last one.
  REQUIRE(playlist.retreat() == entries.at(2));
  REQUIRE(playlist.position() == 3);
  REQUIRE(playlist.retreat() == entries.at(1));
  REQUIRE(playlist.position() == 2);
}

TEST_CASE("A shuffled playlist shows every entry exactly once per cycle", "[playlist]")
{
  const int count = 25;
  const QStringList entries = names(count);
  imgview::Playlist playlist(entries, imgview::PlaylistOrder::Shuffled);

  const QStringList cycle = shownFromHere(playlist, count);

  // The same entries, in some other order.
  QStringList sorted = cycle;
  sorted.sort();
  QStringList entriesSorted = entries;
  entriesSorted.sort();
  REQUIRE(sorted == entriesSorted);
  REQUIRE(QSet<QString>(cycle.begin(), cycle.end()).size() == count);
  // 25 entries have 25! orders; a shuffle that keeps the given order is a bug.
  REQUIRE(cycle != entries);
}

TEST_CASE("Shuffled cycles never repeat an image across the loop boundary", "[playlist]")
{
  const int count = 5;
  imgview::Playlist playlist(names(count), imgview::PlaylistOrder::Shuffled);

  QString previous = playlist.current();
  QSet<QString> cycle{ previous };

  for (int step = 0; step < 500; ++step) {
    const QString current = playlist.advance();

    // Every step, boundary or not, shows a different image than the step before.
    REQUIRE(current != previous);

    if (playlist.position() == 1) {
      // A new cycle has just begun, so the one that ended held every entry once.
      REQUIRE(cycle.size() == count);
      cycle.clear();
    }

    REQUIRE(!cycle.contains(current));
    cycle.insert(current);
    previous = current;
  }
}

TEST_CASE("Stepping backwards into a new shuffled cycle does not repeat the image either", "[playlist]")
{
  const int count = 6;
  const QStringList entries = names(count);
  imgview::Playlist playlist(entries, imgview::PlaylistOrder::Shuffled);

  const QString shown = playlist.current();
  const QString previous = playlist.retreat();

  REQUIRE(!previous.isEmpty());
  REQUIRE(previous != shown);
  REQUIRE(playlist.position() == count);

  // The cycle we stepped into is a permutation too, so the next `count` steps
  // backwards cover every image.
  QSet<QString> cycle{ previous };
  for (int i = 1; i < count; ++i) { cycle.insert(playlist.retreat()); }
  REQUIRE(cycle.size() == count);
}

TEST_CASE("Stepping forward then back returns to the same image", "[playlist]")
{
  imgview::Playlist playlist(names(6), imgview::PlaylistOrder::Shuffled);

  playlist.advance();
  playlist.advance();
  const QString third = playlist.current();

  const QString second = playlist.retreat();
  REQUIRE(second != third);
  REQUIRE(playlist.advance() == third);
}

TEST_CASE("The same seed shuffles the same way", "[playlist]")
{
  QRandomGenerator firstRandom(2024);
  QRandomGenerator secondRandom(2024);

  imgview::Playlist first(names(12), imgview::PlaylistOrder::Shuffled, &firstRandom);
  imgview::Playlist second(names(12), imgview::PlaylistOrder::Shuffled, &secondRandom);

  for (int step = 0; step < 50; ++step) {
    REQUIRE(first.current() == second.current());
    first.advance();
    second.advance();
  }
}

TEST_CASE("Different seeds shuffle differently", "[playlist]")
{
  QRandomGenerator firstRandom(1);
  QRandomGenerator secondRandom(2);

  imgview::Playlist first(names(12), imgview::PlaylistOrder::Shuffled, &firstRandom);
  imgview::Playlist second(names(12), imgview::PlaylistOrder::Shuffled, &secondRandom);

  bool differs = false;
  for (int step = 0; step < 12; ++step) {
    differs = differs || (first.current() != second.current());
    first.advance();
    second.advance();
  }
  REQUIRE(differs);
}

TEST_CASE("restart() draws a new cycle instead of repeating the image on screen", "[playlist]")
{
  imgview::Playlist shuffled(names(8), imgview::PlaylistOrder::Shuffled);
  const QString shown = shuffled.current();

  REQUIRE(shuffled.restart() != shown);
  REQUIRE(shuffled.position() == 1);

  // A sequential playlist has nothing to draw: restarting it goes back to the
  // first entry.
  const QStringList entries = names(3);
  imgview::Playlist sequential(entries, imgview::PlaylistOrder::Sequential);
  sequential.advance();
  REQUIRE(sequential.restart() == entries.at(0));
  REQUIRE(sequential.position() == 1);
}

TEST_CASE("A playlist with a single entry keeps showing it", "[playlist]")
{
  const QString only = QStringLiteral("only.png");
  imgview::Playlist playlist(QStringList{ only }, imgview::PlaylistOrder::Shuffled);

  REQUIRE(playlist.size() == 1);
  REQUIRE(playlist.advance() == only);
  REQUIRE(playlist.retreat() == only);
  REQUIRE(playlist.restart() == only);
  REQUIRE(playlist.position() == 1);
}
