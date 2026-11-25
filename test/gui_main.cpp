// The GUI tests need a QApplication, the playlist and file tests do not. Catch2's
// own main() would only give a QCoreApplication, so this one is ours.

#include <QApplication>
#include <QString>
#include <QStringList>

#include <catch2/catch_session.hpp>

namespace {

//! Whether Catch2 only wants the list of test cases, which it asks for while the
//! project is being built (catch_discover_tests) on machines that may have no
//! display at all. Listing tests needs no window system.
bool wantsOnlyListing(int argc, char **argv)
{
  static const QStringList listingOptions{
    QStringLiteral("--list-tests"),
    QStringLiteral("--list-test-names-only"),
    QStringLiteral("--list-tags"),
    QStringLiteral("--list-reporters"),
    QStringLiteral("--list-listeners"),
  };

  for (int i = 1; i < argc; ++i) {
    if (listingOptions.contains(QString::fromLocal8Bit(argv[i]))) { return true; }
  }
  return false;
}

}// namespace

int main(int argc, char **argv)
{
  if (wantsOnlyListing(argc, argv)) {
    QCoreApplication application(argc, argv);
    return Catch::Session().run(argc, argv);
  }

  QApplication application(argc, argv);
  return Catch::Session().run(argc, argv);
}
