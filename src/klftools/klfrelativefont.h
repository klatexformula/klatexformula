/***************************************************************************
 *   file klfrelativefont.h
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

#ifndef KLFRELATIVEFONT_H
#define KLFRELATIVEFONT_H

#include <QObject>
#include <QWidget>
#include <QFont>

#include <klfdefs.h>


class KLF_EXPORT KLFRelativeFontBase : public QObject
{
  Q_OBJECT
public:
  KLFRelativeFontBase(QWidget *parent);
  virtual ~KLFRelativeFontBase();

  bool eventFilter(QObject *object, QEvent *event);

protected:
  virtual QFont calculateRelativeFont(const QFont& baseFont) = 0;

  inline QWidget * parentWidget() { return qobject_cast<QWidget*>(parent()); }
};


// ----


class KLF_EXPORT KLFRelativeFont : public KLFRelativeFontBase
{
  Q_OBJECT
public:
  KLFRelativeFont(QWidget *parent);
  virtual ~KLFRelativeFont();

  inline QString forceFamily() const { return pForceFamily; }
  inline int relPointSize() const { return pRelPointSize; }

  void setRelPointSize(int relps);
  void setForceFamily(const QString& family);
  void releaseForceFamily() { setForceFamily(QString()); }

protected:
  virtual QFont calculateRelativeFont(const QFont& baseFont);

private:
  QString pForceFamily;
  int pRelPointSize;
};


#endif
