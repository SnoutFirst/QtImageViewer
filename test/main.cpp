// Catch2 normally brings its own main(); the code under test uses Qt, and the
// Qt image plugins want a QCoreApplication to exist, so this one is ours.

#include <QCoreApplication>

#include <catch2/catch_session.hpp>

int main(int argc, char *argv[])
{
  // qApp points at this object as a QCoreApplication *, so it cannot be const.
  QCoreApplication application(argc, argv);// NOLINT(misc-const-correctness)
  return Catch::Session().run(argc, argv);
}
