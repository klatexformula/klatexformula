/***************************************************************************
 *   file klfflowlistwidget.cpp
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

#include <QWidget>
#include <QLabel>

#include <klfdefs.h>

#include "klfflowlayout.h"
#include "klfflowlistwidget.h"
#include "klfflowlistwidget_p.h"


KLFFlowListWidget::KLFFlowListWidget(QWidget *parent) : QWidget(parent)
{
  pLayout = new KLFFlowLayout(this);
  pWidgets = QList<QWidget*>();

  pLayout->setFlush(KLFFlowLayout::FlushBegin);

  setMinimumSize(1,1);
}
KLFFlowListWidget::~KLFFlowListWidget()
{
}


QString KLFFlowListWidget::itemAt(int i) const
{
  KLF_ASSERT_CONDITION(i >= 0 && i < pWidgets.size(),
		       "index "<<i<<" out of bounds [0,"<<pWidgets.size()<<"] !",
		       return QString(); ) ;
  return pWidgets[i]->property("klfflowlistwidget_str").toString();
}
QVariant KLFFlowListWidget::itemDataAt(int i) const
{
  KLF_ASSERT_CONDITION(i >= 0 && i < pWidgets.size(),
		       "index "<<i<<" out of bounds [0,"<<pWidgets.size()<<"] !",
		       return QString(); ) ;
  return pWidgets[i]->property("klfflowlistwidget_data");
}

QStringList KLFFlowListWidget::itemList() const
{
  QStringList s;
  int k;
  for (k = 0; k < pWidgets.size(); ++k) {
    s << itemAt(k);
  }
  return s;
}
QVariantList KLFFlowListWidget::itemDataList() const
{
  QVariantList v;
  int k;
  for (k = 0; k < pWidgets.size(); ++k) {
    v << itemDataAt(k);
  }
  return v;
}

void KLFFlowListWidget::setItems(const QStringList& strings, const QVariantList& datalist)
{
  // clean pWidgets
  while (pWidgets.size()) {
    delete pWidgets.takeAt(0);
  }

  KLF_ASSERT_CONDITION(strings.size() >= datalist.size(),
		       "datalist is larger than strings; some datas will be ignored.", ; ) ;
  // and individually add all items
  int k;
  for (k = 0; k < strings.size(); ++k) {
    QVariant v = (k < datalist.size()) ? datalist[k] : QVariant();
    addItem(strings[k], v);
  }
}
void KLFFlowListWidget::addItem(const QString& string, const QVariant& data)
{
  insertItem(-1, string, data);
}

void KLFFlowListWidget::removeItem(int i)
{
  i = simple_wrapped_item_index(i);
  KLF_ASSERT_CONDITION(i >= 0 && i < pWidgets.size(),
		       "index "<<i<<" out of bounds [0,"<<pWidgets.size()-1<<"] !",
		       return; ) ;

  delete pWidgets[i];
  pWidgets.removeAt(i);
}

void KLFFlowListWidget::insertItem(int i, const QString& s, const QVariant& data)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("at pos="<<i<<", s="<<s<<", data="<<data) ;

  i = simple_wrapped_item_index(i);
  KLF_ASSERT_CONDITION(i >= 0 && i <= pWidgets.size(),
		       "index "<<i<<" out of bounds [0,"<<(pWidgets.size()-1)+1<<"] !",
		       return; ) ;

  KLFFlowListItemWidget *w = new KLFFlowListItemWidget(this);
  QLabel *l = new QLabel(s, this);
  w->setItemWidget(l);

  w->setProperty("klfflowlistwidget_str", QVariant(s));
  w->setProperty("klfflowlistwidget_data", data);

  connect(w, SIGNAL(closeClicked()), this, SLOT(itemClosed()));

  pLayout->addWidget(w, 0, 0, Qt::AlignVCenter);

  pWidgets.insert(i, w);
}

void KLFFlowListWidget::itemClosed()
{
  KLFFlowListItemWidget *w = qobject_cast<KLFFlowListItemWidget*>(sender());
  KLF_ASSERT_NOT_NULL(w, "sender is not a KLFFlowListItemWidget !", return; ) ;

  klfDbg("removing item "<<w<<" ...") ;

  pWidgets.removeAll(w);
  w->deleteLater();
}
