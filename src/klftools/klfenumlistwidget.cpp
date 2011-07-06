/***************************************************************************
 *   file klfenumlistwidget.cpp
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

#include <QUrl>
#include <QTextDocument>

#include "klfenumlistwidget.h"


KLFEnumListWidget::KLFEnumListWidget(QWidget *parent)
  : QLabel(QString(), parent)
{
  setTextFormat(Qt::RichText);
  setWordWrap(true);

  connect(this, SIGNAL(linkActivated(const QString&)), this, SLOT(labelActionLink(const QString&)));
}

KLFEnumListWidget::~KLFEnumListWidget()
{
}

QString KLFEnumListWidget::itemAt(int i) const
{
  if (i >= 0 && i < pItems.size())
    return pItems[i].s;
  return QString();
}

QVariant KLFEnumListWidget::itemDataAt(int i) const
{
  if (i >= 0 && i < pItems.size())
    return pItems[i].data;
  return QVariant();
}

QStringList KLFEnumListWidget::itemList() const
{
  QStringList l;
  int k;
  for (k = 0; k < pItems.size(); ++k)
    l << pItems[k].s;
  return l;
}
QVariantList KLFEnumListWidget::itemDataList() const
{
  QVariantList l;
  int k;
  for (k = 0; k < pItems.size(); ++k)
    l << pItems[k].data;
  return l;
}


void KLFEnumListWidget::removeItem(int i)
{
  i = simple_wrapped_item_index(i);
  if (i < 0 || i >= pItems.size())
    return;

  pItems.removeAt(i);

  updateLabelText();
}
void KLFEnumListWidget::insertItem(int i, const QString& s, const QVariant& data)
{
  i = simple_wrapped_item_index(i);
  pItems.insert(i, Item(s, data));

  updateLabelText();
}

void KLFEnumListWidget::setItems(const QStringList& slist, const QVariantList& datalist)
{
  pItems.clear();
  int k;
  for (k = 0; k < slist.size(); ++k) {
    QVariant d;
    if (k < datalist.size())
      d = datalist[k];
    pItems << Item(slist[k], d);
  }
  if (k < datalist.size()) {
    qWarning()<<KLF_FUNC_NAME<<": Ignoring superfluous elements in data QVariantList";
  }

  updateLabelText();
}

void KLFEnumListWidget::updateLabelText()
{
  QString t;
  t = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
    "<html><head><meta name=\"qrichtext\" content=\"1\" />\n"
    "<style type=\"text/css\">\n"
    ".item { white-space: nowrap }\n"
    "a.itemtext { color: black; text-decoration: underline; }\n"
    "a.actionlink { color: blue; text-decoration: none; font-weight: bold; }\n"
    "</style></head><body>\n"
    "<p>";
  int k;
  for (k = 0; k < pItems.size(); ++k) {
    t += "<span class=\"item\"><a class=\"itemtext\" href=\"klfenumlistwidgetaction:/itemClick?i="
      +QString::number(k)+"\">" + Qt::escape(pItems[k].s) + "</a>&nbsp;";
    t += "<a class=\"actionlink\" href=\"klfenumlistwidgetaction:/removeAt?i="+QString::number(k)+"\">[-]</a>"
      + "&nbsp;&nbsp;</span> ";
  }
  t += "</p>"   "</body></html>";

  setText(t);
}

void KLFEnumListWidget::labelActionLink(const QString& link)
{
  QUrl url = QUrl::fromEncoded(link.toLatin1());

  klfDbg("link clicked="<<link<<" scheme="<<url.scheme()<<", path="<<url.path()) ;

  if (url.scheme() == "klfenumlistwidgetaction") {
    if (url.path() == "/removeAt") {
      int i = url.queryItemValue("i").toInt();
      removeItem(i);
      return;
    }
    if (url.path() == "/itemClick") {
      int i = url.queryItemValue("i").toInt();
      emit itemActivated(i, itemAt(i), itemDataAt(i));
      emit itemActivated(itemAt(i), itemDataAt(i));
      return;
    }
  }

  klfDbg("don't know what to do with link: "<<link<<" ...") ;
}



