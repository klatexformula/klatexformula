/***************************************************************************
 *   file klfdbus.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#include "klfmainwin.h"
#include "klfdbus.h"


KLFDBusAppAdaptor::KLFDBusAppAdaptor(QApplication *application, KLFMainWin *mainWin)
  : QDBusAbstractAdaptor(application), app(application), _mainwin(mainWin)
{
}


KLFDBusAppAdaptor::~KLFDBusAppAdaptor()
{
}

void KLFDBusAppAdaptor::raiseWindow()
{
  if ( ! _mainwin->isVisible() )
    _mainwin->show();

  _mainwin->setWindowState(_mainwin->windowState() & ~Qt::WindowMinimized);
  _mainwin->raise();
  _mainwin->activateWindow();
}

Q_NOREPLY void KLFDBusAppAdaptor::quit()
{
  app->quit();
}

void KLFDBusAppAdaptor::setInputData(const QString& key, const QString& svalue, int ivalue)
{
  if (key == "latex") {
    _mainwin->slotSetLatex(svalue);
  } else if (key == "fgcolor") {
    _mainwin->slotSetFgColor(svalue);
  } else if (key == "bgcolor") {
    _mainwin->slotSetBgColor(svalue);
  } else if (key == "mathmode") {
    _mainwin->slotSetMathMode(svalue);
  } else if (key == "preamble") {
    _mainwin->slotSetPreamble(svalue);
  } else if (key == "dpi") {
    _mainwin->slotSetDPI(ivalue);
  }
}

void KLFDBusAppAdaptor::setAlterSetting_i(int setting, int value)
{
  _mainwin->alterSetting((KLFMainWin::altersetting_which)setting, value);
}
void KLFDBusAppAdaptor::setAlterSetting_s(int setting, const QString& value)
{
  _mainwin->alterSetting((KLFMainWin::altersetting_which)setting, value);
}

void KLFDBusAppAdaptor::evaluateAndSave(const QString& output, const QString& fmt)
{
  _mainwin->slotEvaluateAndSave(output, fmt);
}

void KLFDBusAppAdaptor::openFile(const QString& fileName)
{
  klfDbg("opening file: "<<fileName) ;
  _mainwin->openFile(fileName);
}
void KLFDBusAppAdaptor::openFiles(const QStringList& fileNameList)
{
  klfDbg("opening files: "<<fileNameList) ;
  _mainwin->openFiles(fileNameList);
}
void KLFDBusAppAdaptor::openData(const QByteArray& data)
{
  klfDbg("opening data: length="<<data.length()<<", data="<<data) ;
  _mainwin->openData(data);
}

void KLFDBusAppAdaptor::importCmdlKLFFiles(const QStringList& files)
{
  klfDbg("opening files: "<<files) ;
  _mainwin->openLibFiles(files);
}



KLFDBusAppInterface::KLFDBusAppInterface(const QString &service, const QString &path,
					 const QDBusConnection &connection, QObject *parent)
     : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

KLFDBusAppInterface::~KLFDBusAppInterface()
{
}

QDBusReply<void> KLFDBusAppInterface::quit()
{
  QList<QVariant> argumentList;
  return callWithArgumentList(QDBus::Block, QString("quit"), argumentList);
}

QDBusReply<void> KLFDBusAppInterface::raiseWindow()
{
  QList<QVariant> argumentList;
  return callWithArgumentList(QDBus::Block, QString("raiseWindow"), argumentList);
}

QDBusReply<void> KLFDBusAppInterface::setInputData(const QString& key, const QString& svalue, int ivalue)
{
  QList<QVariant> argumentList;
  argumentList << QVariant(key) << QVariant(svalue) << QVariant(ivalue);
  return callWithArgumentList(QDBus::Block, QString("setInputData"), argumentList);
}

QDBusReply<void> KLFDBusAppInterface::setAlterSetting_i(int setting, int value)
{
  QList<QVariant> argumentList;
  argumentList << QVariant(setting) << QVariant(value);
  return callWithArgumentList(QDBus::Block, QString("setAlterSetting_i"), argumentList);
}

QDBusReply<void> KLFDBusAppInterface::setAlterSetting_s(int setting, const QString& value)
{
  QList<QVariant> argumentList;
  argumentList << QVariant(setting) << QVariant(value);
  return callWithArgumentList(QDBus::Block, QString("setAlterSetting_s"), argumentList);
}

QDBusReply<void> KLFDBusAppInterface::evaluateAndSave(const QString& output, const QString& fmt)
{
  return callWithArgumentList( QDBus::Block, QString("evaluateAndSave"),
			       QList<QVariant>() << QVariant(output) << QVariant(fmt) );
}

QDBusReply<void> KLFDBusAppInterface::openFile(const QString& fileName)
{
  return callWithArgumentList( QDBus::Block, QString("openFile"),
			       QList<QVariant>() << QVariant(fileName) );
}
QDBusReply<void> KLFDBusAppInterface::openFiles(const QStringList& fileNameList)
{
  return callWithArgumentList( QDBus::Block, QString("openFiles"),
			       QList<QVariant>() << QVariant(fileNameList) );
}
QDBusReply<void> KLFDBusAppInterface::openData(const QByteArray& data)
{
  return callWithArgumentList( QDBus::Block, QString("openData"),
			       QList<QVariant>() << QVariant(data) );
}

QDBusReply<void> KLFDBusAppInterface::importCmdlKLFFiles(const QStringList& fnames)
{
  QList<QVariant> argumentList;
  argumentList << QVariant(fnames);
  return callWithArgumentList(QDBus::Block, QString("importCmdlKLFFiles"), argumentList);
}
