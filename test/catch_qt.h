// Makes Catch2 print QString and QStringList in its failure messages instead of
// "{?}". Has to be included before the first assertion in a translation unit.

#ifndef QTIMAGEVIEWER_CATCH_QT_H
#define QTIMAGEVIEWER_CATCH_QT_H

#include <QString>
#include <QStringList>

#include <catch2/catch_tostring.hpp>

#include <string>

namespace Catch {

template<> struct StringMaker<QString>
{
  static std::string convert(const QString &value) { return value.toStdString(); }
};

template<> struct StringMaker<QStringList>
{
  static std::string convert(const QStringList &value) { return value.join(QStringLiteral(", ")).toStdString(); }
};

}// namespace Catch

#endif
