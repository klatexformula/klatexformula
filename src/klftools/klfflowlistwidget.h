/***************************************************************************
 *   file klfflowlistwidget.h
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

#ifndef KLFFLOWLISTWIDGET_H
#define KLFFLOWLISTWIDGET_H

#include <QVariant>
#include <QWidget>

class QLabel;
class KLFFlowLayout;
class KLFFlowListItemWidget;

class KLFFlowListWidget : public QWidget
{
  Q_OBJECT
public:
  KLFFlowListWidget(QWidget *parent = NULL);
  virtual ~KLFFlowListWidget();  

  inline int itemCount() const { return pWidgets.size(); }

  QString itemAt(int i) const;
  QVariant itemDataAt(int i) const;

  QStringList itemList() const;
  QVariantList itemDataList() const;

signals:
  void itemActivated(const QString& s, const QVariant& data);
  void itemActivated(int i, const QString& s, const QVariant& data);

public slots:
  void setItems(const QStringList& strings, const QVariantList& datalist = QVariantList());
  void addItem(const QString& string, const QVariant& data = QVariant());

  void removeItem(int i);
  void insertItem(int i, const QString& s, const QVariant& data = QVariant()) ;

private slots:
  void itemClosed();

private:
  KLFFlowLayout *pLayout;

  QList<QWidget*> pWidgets;

  inline int simple_wrapped_item_index(int i) const { return (i >= 0) ? i : pWidgets.size() - (-i) + 1; }
};




#endif
