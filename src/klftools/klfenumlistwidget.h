/***************************************************************************
 *   file klfenumlistwidget.h
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


#ifndef KLF_ENUMLISTWIDGET_H
#define KLF_ENUMLISTWIDGET_H

#include <QWidget>
#include <QLabel>

#include <klfdefs.h>


class KLF_EXPORT KLFEnumListWidget : public QLabel
{
  Q_OBJECT
public:
  KLFEnumListWidget(QWidget *parent);
  virtual ~KLFEnumListWidget();

  inline int itemCount() const { return pItems.size(); }

  QString itemAt(int i) const;
  QVariant itemDataAt(int i) const;

  QStringList itemList() const;
  QVariantList itemDataList() const;

signals:
  void itemActivated(const QString& s, const QVariant& data);
  void itemActivated(int i, const QString& s, const QVariant& data);

public slots:
  void addItem(const QString& s, const QVariant& data = QVariant()) { insertItem(-1, s, data); }

  void removeItem(int i);
  void insertItem(int i, const QString& s, const QVariant& data = QVariant()) ;

  void setItems(const QStringList& slist, const QVariantList& datalist = QVariantList());

protected:
  virtual void updateLabelText();

  struct Item {
    Item(const QString& s_ = QString(), const QVariant& d_ = QVariant())  :  s(s_), data(d_)  { }
    QString s;
    QVariant data;
  };

  QList<Item> pItems;

private slots:

  void labelActionLink(const QString& link);

private:
  inline int simple_wrapped_item_index(int i) const { return (i >= 0) ? i : pItems.size() + i; }
};





#endif
