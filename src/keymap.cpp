#include "keymap.h"

#include <QStringList>

#include <QtCore/qnamespace.h>

#include <algorithm>
#include <array>
#include <vector>

namespace imgview {
namespace {

/*
 * The order of this table is the order of the context menu and of the "Keyboard
 * shortcuts" section of --help; the keys of one command sit together, and the
 * first of them is the one the menu shows next to the action.
 *
 * The Q_OBJECT-free part of the program only needs Qt::Key and QString, so this
 * lives with the core rather than with the widgets -- but it is still the table
 * the window's keyPressEvent is driven from (commandForKey()), which is what
 * keeps the documentation and the behaviour the same thing.
 */
constexpr std::array kBindings{ KeyBinding{ Qt::Key_Right, "Right", Command::NextImage },
  KeyBinding{ Qt::Key_Down, "Down", Command::NextImage },
  KeyBinding{ Qt::Key_Space, "Space", Command::NextImage },
  KeyBinding{ Qt::Key_PageDown, "Page Down", Command::NextImage },
  KeyBinding{ Qt::Key_N, "n", Command::NextImage },

  KeyBinding{ Qt::Key_Left, "Left", Command::PreviousImage },
  KeyBinding{ Qt::Key_Up, "Up", Command::PreviousImage },
  KeyBinding{ Qt::Key_Backspace, "Backspace", Command::PreviousImage },
  KeyBinding{ Qt::Key_PageUp, "Page Up", Command::PreviousImage },
  KeyBinding{ Qt::Key_P, "p", Command::PreviousImage },

  KeyBinding{ Qt::Key_Return, "Return", Command::ToggleSlideshow },
  KeyBinding{ Qt::Key_Enter, "keypad Enter", Command::ToggleSlideshow },
  KeyBinding{ Qt::Key_S, "s", Command::ToggleSlideshow },

  KeyBinding{ Qt::Key_F, "f", Command::ToggleFullScreen },
  KeyBinding{ Qt::Key_F11, "F11", Command::ToggleFullScreen },

  KeyBinding{ Qt::Key_V, "v", Command::ToggleVerbose },

  KeyBinding{ Qt::Key_R, "r", Command::RestartCycle },

  KeyBinding{ Qt::Key_Plus, "+", Command::ZoomIn },
  KeyBinding{ Qt::Key_Equal, "=", Command::ZoomIn },

  KeyBinding{ Qt::Key_Minus, "-", Command::ZoomOut },
  KeyBinding{ Qt::Key_Underscore, "_", Command::ZoomOut },

  KeyBinding{ Qt::Key_Q, "q", Command::Quit } };

}// namespace

std::span<const KeyBinding> keyBindings() { return kBindings; }

std::span<const Command> commands()
{
  // Worked out from the table rather than written out again, so the two cannot
  // disagree: the commands are in the order their first binding appears in.
  static const std::vector<Command> kCommands = []
  {
    std::vector<Command> unique;
    for (const KeyBinding &binding : kBindings) {
      if (std::find(unique.begin(), unique.end(), binding.command) == unique.end()) {
        unique.push_back(binding.command);
      }
    }
    return unique;
  }();
  return kCommands;
}

QString commandDescription(Command command)
{
  switch (command) {
  case Command::NextImage:
    return QStringLiteral("Next image");
  case Command::PreviousImage:
    return QStringLiteral("Previous image");
  case Command::ToggleSlideshow:
    return QStringLiteral("Start or stop the slideshow");
  case Command::ToggleFullScreen:
    return QStringLiteral("Full screen");
  case Command::ToggleVerbose:
    return QStringLiteral("Show or hide the details over the image");
  case Command::RestartCycle:
    return QStringLiteral("Start the list over (a new shuffle with --random)");
  case Command::ZoomIn:
    return QStringLiteral("Zoom in");
  case Command::ZoomOut:
    return QStringLiteral("Zoom out");
  case Command::Quit:
    return QStringLiteral("Quit");
  }
  return {};
}

QString keysFor(Command command)
{
  QStringList names;
  for (const KeyBinding &binding : kBindings) {
    if (binding.command == command) { names.append(QString::fromLatin1(binding.name)); }
  }
  return names.join(QStringLiteral(", "));
}

int primaryKey(Command command)
{
  const auto *const binding = std::find_if(
    kBindings.begin(), kBindings.end(), [command](const KeyBinding &candidate) { return candidate.command == command; });

  // Unreachable: every command in the enum has a binding in the table above.
  return binding == kBindings.end() ? 0 : binding->key;
}

std::optional<Command> commandForKey(int key)
{
  const auto *const binding = std::find_if(
    kBindings.begin(), kBindings.end(), [key](const KeyBinding &candidate) { return candidate.key == key; });

  if (binding == kBindings.end()) { return std::nullopt; }
  return binding->command;
}

}// namespace imgview
