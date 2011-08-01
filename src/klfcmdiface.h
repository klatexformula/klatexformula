/***************************************************************************
 *   file klfcmdiface.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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


#ifndef KLF_CMDIFACE_H_
#define KLF_CMDIFACE_H_

#include <QObject>
#include <QString>
#include <QUrl>

#include <klfdefs.h>

class KLFCmdIfacePrivate;

class KLF_EXPORT KLFCmdIface : public QObject
{
  Q_OBJECT
public:
  struct Command {
    Command(const QString& o = QString(), const QString& s = QString(),
	    const QVariantList& a = QVariantList())
      : object(o), slot(s), args(a) { }
    Command(const Command& copy) : object(copy.object), slot(copy.slot), args(copy.args) { }

    QString object;
    QString slot;
    QVariantList args;
  };

  KLFCmdIface(QObject *parent = NULL);
  ~KLFCmdIface();

  void registerObject(QObject *target, const QString& objectHostName);

  static QUrl encodeCommand(const Command& command);
  static Command decodeCommand(const QUrl& url);

public slots:
  void executeCommand(const QUrl& url);

private:
  KLF_DECLARE_PRIVATE(KLFCmdIface) ;
};



#endif
