#include "keymap.h"

#include <catch2/catch_test_macros.hpp>

#include <QtCore/qnamespace.h>

#include <algorithm>
#include <set>
#include <vector>

namespace {

//! What has to hold for every command, whatever key the table gives it.
void checkCommand(imgview::Command command)
{
  // The context menu shows primaryKey() and the window handles commandForKey():
  // if those two disagree, the menu is telling the user a key that does not work.
  const int key = imgview::primaryKey(command);
  REQUIRE(key != 0);
  REQUIRE(imgview::commandForKey(key) == command);

  REQUIRE_FALSE(imgview::keysFor(command).isEmpty());
  REQUIRE_FALSE(imgview::commandDescription(command).isEmpty());
}

//! The commands in the order the table first mentions them, worked out by hand
//! rather than by commands(), which is what is being checked.
std::vector<imgview::Command> firstAppearances()
{
  std::vector<imgview::Command> commands;
  for (const imgview::KeyBinding &binding : imgview::keyBindings()) {
    if (std::find(commands.begin(), commands.end(), binding.command) == commands.end()) {
      commands.push_back(binding.command);
    }
  }
  return commands;
}

}// namespace

TEST_CASE("Every command has at least one key, and the key runs it", "[keymap]")
{
  REQUIRE_FALSE(imgview::commands().empty());

  for (const imgview::Command command : imgview::commands()) { checkCommand(command); }
}

TEST_CASE("Every key in the table runs the command it is listed under", "[keymap]")
{
  for (const imgview::KeyBinding &binding : imgview::keyBindings()) {
    REQUIRE(imgview::commandForKey(binding.key) == binding.command);
  }
}

TEST_CASE("The commands are the bindings, in the order they first appear", "[keymap]")
{
  // --help and the context menu both walk commands(), so it has to be exactly
  // the table's own commands, once each, in the table's order.
  const std::vector<imgview::Command> expected = firstAppearances();

  REQUIRE(expected.size() == imgview::commands().size());
  REQUIRE(std::equal(expected.begin(), expected.end(), imgview::commands().begin()));
}

TEST_CASE("No key is claimed twice", "[keymap]")
{
  std::set<int> keys;
  for (const imgview::KeyBinding &binding : imgview::keyBindings()) { REQUIRE(keys.insert(binding.key).second); }
}

TEST_CASE("Escape is the window's own, and unknown keys belong to nobody", "[keymap]")
{
  // Escape leaves full screen, and only quits when there is none. It is not a
  // menu entry, so it is deliberately not in the table.
  REQUIRE_FALSE(imgview::commandForKey(Qt::Key_Escape).has_value());
  REQUIRE_FALSE(imgview::commandForKey(Qt::Key_Z).has_value());
}
