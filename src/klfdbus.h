/***************************************************************************
 *   file klfdbus.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id$ */

#ifndef KLFDBUS_H
#define KLFDBUS_H

#if defined(KLF_USE_DBUS)

#include <QDBusConnection>
#include <QDBusAbstractAdaptor>
#include <QDBusAbstractInterface>
#include <QDBusReply>
#include <QApplication>

#include <klfdefs.h>

class KLFMainWin;


class KLF_EXPORT KLFDBusAppAdaptor : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.klatexformula.KLatexFormula")

private:
  QApplication *app;
  KLFMainWin *_mainwin;

public:
  KLFDBusAppAdaptor(QApplication *application, KLFMainWin *mainWin);
  virtual ~KLFDBusAppAdaptor();

public slots:
  Q_NOREPLY void quit();

  void raiseWindow();

  void setInputData(const QString& key, const QString& svalue, int ivalue);
  void setAlterSetting_i(int setting, int value);
  void setAlterSetting_s(int setting, const QString& value);
  void evaluateAndSave(const QString& output, const QString& fmt);

  void openFile(const QString& fileName);
  void openFiles(const QStringList& fileNameList);
  void openData(const QByteArray& data);

  void importCmdlKLFFiles(const QStringList& fnames);
};


class KLF_EXPORT KLFDBusAppInterface: public QDBusAbstractInterface
{
  Q_OBJECT
public:
  static inline const char *staticInterfaceName()
  {
    return "org.klatexformula.KLatexFormula";
  }
  
public:
  KLFDBusAppInterface(const QString &service, const QString &path, const QDBusConnection &connection,
		      QObject *parent = 0);
  ~KLFDBusAppInterface();

public slots: // METHODS

  QDBusReply<void> quit();
  QDBusReply<void> raiseWindow();
  QDBusReply<void> setInputData(const QString& key, const QString& svalue, int ivalue = -1);
  QDBusReply<void> setAlterSetting_i(int setting, int value);
  QDBusReply<void> setAlterSetting_s(int setting, const QString& value);
  QDBusReply<void> evaluateAndSave(const QString& output, const QString& fmt);
  QDBusReply<void> openFile(const QString& fileName);
  QDBusReply<void> openFiles(const QStringList& fileNameList);
  QDBusReply<void> openData(const QByteArray& data);
  QDBusReply<void> importCmdlKLFFiles(const QStringList& fnames);

};


#endif

#endif
