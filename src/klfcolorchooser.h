/***************************************************************************
 *   file klfcolorchooser.h
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

#ifndef KLFCOLORCHOOSER_H
#define KLFCOLORCHOOSER_H

#include <QPushButton>
#include <QColor>
#include <QApplication>
#include <QList>


class KLFColorList : public QObject
{
  Q_OBJECT
public:
  KLFColorList() : QObject(qApp) { }

  QList<QColor> list;

signals:
  void listChanged();

public slots:
  void addColor(const QColor& color);
  void notifyListChanged() { emit listChanged(); }
};


class KLFColorChooser : public QPushButton
{
  Q_OBJECT

  Q_PROPERTY(QSize showSize READ showSize WRITE setShowSize)
  Q_PROPERTY(bool allowDefaultState READ allowDefaultState WRITE setAllowDefaultState)
  Q_PROPERTY(bool autoAddToList READ autoAddToList WRITE setAutoAddToList)
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)

public:
  KLFColorChooser(QWidget *parent);
  ~KLFColorChooser();

  QSize showSize() const { return _size; }
  bool allowDefaultState() const { return _allowdefaultstate; }
  bool autoAddToList() const { return _autoadd; }
  QColor color() const;

  static void ensureColorListInstance();
  static void setColorList(const QList<QColor>& colorlist);
  static QList<QColor> colorList();

public slots:
  void setColor(const QColor& color);
  void setAllowDefaultState(bool allow);
  void setAutoAddToList(bool autoadd) { _autoadd = autoadd; }
  void setShowSize(const QSize& size) { _size = size; }
  void setDefaultColor();

  void requestColor();

protected slots:
  void setSenderPropertyColor();
  void _makemenu();

private:
  QColor _color;
  QPixmap _pix;

  bool _allowdefaultstate;
  bool _autoadd;
  QSize _size;

  QMenu *mMenu;

  void _setpix();

  QPixmap colorPixmap(const QColor& color, const QSize& size);

  static KLFColorList *_colorlist;
};


#endif
