/***************************************************************************
 *   file klfitemviewsearchtarget_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfitemviewsearchtarget.cpp */

#ifndef KLFITEMVIEWSEARCHTARGET_P_H
#define KLFITEMVIEWSEARCHTARGET_P_H

#include <QTextLayout>
#include <QTextOption>
#include <QTextDocument>
#include <QItemDelegate>
#include <QPainter>
#include "klfitemviewsearchtarget.h"


class KLFSearchItemDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  KLFSearchItemDelegate(QObject *parent)
    : QItemDelegate(parent)
  {
    pQueryString = QString();
  }
  virtual ~KLFSearchItemDelegate()
  {
  }

  struct Match {
    int pos;
    int len;
  };

  static QList<Match> matches(const QString& str, const QString& queryString, bool findall = true)
  {
    QList<Match> mlist;
    Match m;
    bool has_uppercase = queryString.contains(QRegExp("[A-Z]"));
    int i = 0;
    for (;;) {
      m.pos = str.indexOf(queryString, i, has_uppercase ? Qt::CaseSensitive : Qt::CaseInsensitive);
      if (m.pos == -1) {
	m.len = -1;
	return mlist;
      }
      m.len = queryString.length();
      i += m.len;
      mlist.append(m);
      if (!findall)
	return mlist;
    }
  }

  void setSearchString(const QString& str)
  {
    pQueryString = str;
  }

protected:
  virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem& option, const QRect& rect,
			   const QString& text) const
  {
    // copied from QItemDelegate

    QPalette::ColorGroup cg = ( (option.state & QStyle::State_Enabled)
				? QPalette::Normal : QPalette::Disabled );
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
      cg = QPalette::Inactive;
    if (option.state & QStyle::State_Selected) {
      painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
      painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
      painter->setPen(option.palette.color(cg, QPalette::Text));
    }
    
    if (text.isEmpty())
      return;

    if (option.state & QStyle::State_Editing) {
      painter->save();
      painter->setPen(option.palette.color(cg, QPalette::Text));
      painter->drawRect(rect.adjusted(0, 0, -1, -1));
      painter->restore();
    }

    const QWidget *widget = NULL;
    QStyle * style = QApplication::style();
    //    if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option)) {
    widget = option.widget;
    style = widget->style();
    //}

    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;
    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = option.features & QStyleOptionViewItem::WrapText;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option.direction);
    textOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
    textLayout.setTextOption(textOption);
    textLayout.setFont(option.font);
    textLayout.setText(replaceNewLine(text));

    // --
    // highlight search matches
    if (pQueryString.size()) {
      QList<Match> mlist = matches(text, pQueryString);
      QTextCharFormat fmt;
      fmt.setFontWeight(QFont::Bold);
      fmt.setForeground(QColor(127,0,0));
      fmt.setBackground(QColor(255,220,200, 100));
      QList<QTextLayout::FormatRange> flist;
      foreach (Match m, mlist) {
	QTextLayout::FormatRange f;
	f.format = fmt;
	f.start = m.pos;
	f.length = m.len;
	flist << f;
      }
      textLayout.setAdditionalFormats(flist);
    }
    // --

    QSizeF textLayoutSize = doTextLayout(textRect.width());

    if (textRect.width() < textLayoutSize.width()
        || textRect.height() < textLayoutSize.height()) {
        QString elided;
        int start = 0;
        int end = text.indexOf(QChar::LineSeparator, start);
        if (end == -1) {
            elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
        } else {
            while (end != -1) {
                elided += option.fontMetrics.elidedText(text.mid(start, end - start),
                                                        option.textElideMode, textRect.width());
                elided += QChar::LineSeparator;
                start = end + 1;
                end = text.indexOf(QChar::LineSeparator, start);
            }
            //let's add the last line (after the last QChar::LineSeparator)
            elided += option.fontMetrics.elidedText(text.mid(start),
                                                    option.textElideMode, textRect.width());
        }
        textLayout.setText(elided);
        textLayoutSize = doTextLayout(textRect.width());
    }

    const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
    const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                                  layoutSize, textRect);
    // if we still overflow even after eliding the text, enable clipping
    if (!hasClipping() && (textRect.width() < textLayoutSize.width()
                           || textRect.height() < textLayoutSize.height())) {
        painter->save();
        painter->setClipRect(layoutRect);
        textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
        painter->restore();
    } else {
        textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
    }

  }

private:
  QString pQueryString;

  inline static QString replaceNewLine(QString text)
  {
    const QChar nl = QLatin1Char('\n');
    for (int i = 0; i < text.count(); ++i)
      if (text.at(i) == nl)
	text[i] = QChar::LineSeparator;
    return text;
  }

  QSizeF doTextLayout(int lineWidth) const
  {
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
      QTextLine line = textLayout.createLine();
      if (!line.isValid())
	break;
      line.setLineWidth(lineWidth);
      line.setPosition(QPointF(0, height));
      height += line.height();
      widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
  }

  mutable QTextLayout textLayout;
  mutable QTextOption textOption;
};





#endif
