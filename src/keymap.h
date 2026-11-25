#ifndef QTIMAGEVIEWER_KEYMAP_H
#define QTIMAGEVIEWER_KEYMAP_H

#include <QString>

#include <optional>
#include <span>

namespace imgview {

//! Everything the keyboard can be told to do. The context menu offers the same set, in this order.
enum class Command {
  NextImage,
  PreviousImage,
  ToggleSlideshow,
  ToggleFullScreen,
  ToggleVerbose,
  RestartCycle,
  ZoomIn,
  ZoomOut,
  Quit,
};

//! One key and what it runs.
struct KeyBinding
{
  int key;//!< A Qt::Key.
  const char *name;//!< How the key is written in --help and in the context menu.
  Command command;
};

/*
 * The one table of shortcuts, defined once in keymap.cpp. --help, the context
 * menu and the key handler all read it, so a key cannot be documented in one
 * place, offered in another and handled in a third.
 */
//! Every key the window handles, grouped by command.
std::span<const KeyBinding> keyBindings();

//! The commands that have keys, in the order --help and the context menu list them.
std::span<const Command> commands();

//! What \a command does, as both of them word it.
QString commandDescription(Command command);

//! Every key that runs \a command, written as --help writes them: "Right, Down, Space, Page Down, n".
QString keysFor(Command command);

//! The key the context menu shows for \a command: its first binding.
int primaryKey(Command command);

//! The command \a key runs, if it runs one.
std::optional<Command> commandForKey(int key);

}// namespace imgview

#endif
