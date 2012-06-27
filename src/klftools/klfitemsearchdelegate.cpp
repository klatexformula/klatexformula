/***************************************************************************
 *   file klfitemsearchdelegate.cpp
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

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTextCharFormat>

#include "klfitemsearchdelegate.h"


struct KLFItemSearchDelegatePrivate
{
  KLF_PRIVATE_HEAD(KLFItemSearchDelegate) {
    matchformat = QTextCharFormat();
    matchformat.setForeground(QColor(128,0,0));
    matchformat.setFontItalic(true);
    matchformat.setFontWeight(QFont::Bold);
    currentmatchformat = matchformat;
    currentmatchformat.setBackground(QColor(0,255,0,80));
  }
  
  QTextCharFormat matchformat;
  QTextCharFormat currentmatchformat;


  void manual_paint_item_with_highlightmatches(QPainter *p, QStyleOptionViewItemV4 *opt,
					       QWidget *widget, QList<int> matchpositions)
  {
    p->save();
    p->setClipRect(opt->rect);

    QStyle *style = widget->style();
    
    KLF_ASSERT_NOT_NULL(style, "Widget's style is NULL!", return ; ) ;

    // CODE TAKEN FROM Qt's source src/gui/styles/qcommonstyle.cpp, function drawControl(), case
    // `CE_ItemViewItem'.

    QRect checkRect = style->subElementRect(SE_ItemViewItemCheckIndicator, opt, widget);
    QRect iconRect = style->subElementRect(SE_ItemViewItemDecoration, opt, widget);
    QRect textRect = style->subElementRect(SE_ItemViewItemText, opt, widget);
    
    // draw the background
    style->drawPrimitive(PE_PanelItemViewItem, opt, p, widget);

    // draw the check mark
    if (opt->features & QStyleOptionViewItemV2::HasCheckIndicator) {
      QStyleOptionViewItemV4 option(*opt);
      option.rect = checkRect;
      option.state = option.state & ~QStyle::State_HasFocus;
      
      switch (opt->checkState) {
      case Qt::Unchecked:
	option.state |= QStyle::State_Off;
	break;
      case Qt::PartiallyChecked:
	option.state |= QStyle::State_NoChange;
	break;
      case Qt::Checked:
	option.state |= QStyle::State_On;
	break;
      }
      style()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &option, p, widget);
    }
    
    // draw the icon
    QIcon::Mode mode = QIcon::Normal;
    if (!(opt->state & QStyle::State_Enabled))
      mode = QIcon::Disabled;
    else if (opt->state & QStyle::State_Selected)
      mode = QIcon::Selected;
    QIcon::State state = opt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
    opt->icon.paint(p, iconRect, opt->decorationAlignment, mode, state);
    
    // draw the text
    if (!opt->text.isEmpty()) {

      // find out the text palette

      QPalette::ColorGroup cg = opt->state & QStyle::State_Enabled
	? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(opt->state & QStyle::State_Active))
	cg = QPalette::Inactive;

      if (opt->state & QStyle::State_Selected) {
	p->setPen(vopt->palette.color(cg, QPalette::HighlightedText));
      } else {
	p->setPen(vopt->palette.color(cg, QPalette::Text));
      }


      // highlight the match positions
      // -----------------------------

............... ........... . . . ..  .. .  . .. . .

      QTextDocument textDocument;
      textDocument.setDefaultFont(opt->font);
      QTextCursor cur(&textDocument);
      QList<ColorRegion> appliedfmts;
      for (i = ci = 0; i < text.length(); ++i) {
	//      klfDbg( "Processing char "<<text[i]<<"; i="<<i<<"; ci="<<ci ) ;
	if (ci >= c.size() && appliedfmts.size() == 0) {
	// insert all remaining text (no more formatting needed)
	cur.insertText(Qt::escape(text.mid(i)), QTextCharFormat());
	break;
      }
      while (ci < c.size() && c[ci].start == i) {
	appliedfmts.append(c[ci]);
	++ci;
      }
      QTextCharFormat f;
      for (j = 0; j < appliedfmts.size(); ++j) {
	if (i >= appliedfmts[j].start + appliedfmts[j].len) {
	  // this format no longer applies
	  appliedfmts.removeAt(j);
	  --j;
	  continue;
	}
	f.merge(appliedfmts[j].fmt);
      }
      cur.insertText(Qt::escape(text[i]), f);
    }

    QSizeF s = textDocument.size();
    QRectF textRect = p->innerRectText;
    // note: setLeft changes width, not right.
    textRect.setLeft(textRect.left()-4); // some offset because of qt's rich text drawing ..?
    // textRect is now correct
    // draw a "hidden children selection" marker, if needed
    if (flags & PTF_SelUnderline) {
       QColor h1 = p->option->palette.color(QPalette::Highlight);
       QColor h2 = h1;
       h1.setAlpha(0);
       h2.setAlpha(220);
       QLinearGradient gr(0.f, 0.f, 0.f, 1.f);
       gr.setCoordinateMode(QGradient::ObjectBoundingMode);
       gr.setColorAt(0.f, h1);
       gr.setColorAt(1.f, h2);
       QBrush selbrush(gr);
       p->p->save();
       //       p->p->setClipRect(p->option->rect);
       p->p->fillRect(QRectF(textRect.left(), textRect.bottom()-10,
			     textRect.width(), 10), selbrush);
       p->p->restore();
    }
    // check the text size
    drawnTextWidth = (int)s.width();
    if (s.width() > textRect.width()) { // sorry, too large
      s.setWidth(textRect.width());
    }
    p->p->save();
    p->p->setClipRect(textRect);
    if (s.height() > textRect.height()) { // contents height exceeds available height
      klfDbg("Need borderfadepen for (rich) text "<<textDocument.toHtml());
      p->p->setPen(borderfadepen);
    } else {
      p->p->setPen(normalpen);
    }
    p->p->translate(textRect.topLeft());
    p->p->translate( QPointF( 0, //textRect.width() - s.width(),
			      textRect.height() - s.height()) / 2.f );
    textDocument.drawContents(p->p);
    p->p->restore();
    drawnBaseLineY = (int)(textRect.bottom() - (textRect.height() - s.height()) / 2.f);
  }

  // draw arrow if text too large

  if (drawnTextWidth > p->innerRectText.width()) {
    // draw small arrow indicating more text
    //      c.setAlpha(80);
    p->p->save();
    p->p->translate(p->option->rect.right()-2, drawnBaseLineY-2);
    p->p->setPen(textcol);
    p->p->drawLine(0, 0, -16, 0);
    p->p->drawLine(0, 0, -2, +2);
    p->p->restore();
  }

      
      d->viewItemDrawText(p, vopt, textRect);
    }
    
    // draw the focus rect
    if (opt->state & QStyle::State_HasFocus) {
      QStyleOptionFocusRect o;
      o.QStyleOption::operator=(*opt);
      o.rect = style->subElementRect(SE_ItemViewItemFocusRect, opt, widget);
      o.state |= QStyle::State_KeyboardFocusChange;
      o.state |= QStyle::State_Item;
      QPalette::ColorGroup cg = (opt->state & QStyle::State_Enabled)
	? QPalette::Normal : QPalette::Disabled;
      o.backgroundColor = vopt->palette.color(cg, (vopt->state & QStyle::State_Selected)
					      ? QPalette::Highlight : QPalette::Window);
      style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, p, widget);
    }
    
    p->restore();
}

};

KLFItemSearchDelegate::KLFItemSearchDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
  KLF_INIT_PRIVATE(KLFItemSearchDelegate) ;
}
KLFItemSearchDelegate::~KLFItemSearchDelegate()
{
  KLF_DELETE_PRIVATE ;
}
void KLFItemSearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option,
				  const QModelIndex& index) const
{
  QList<int> matchpos;
  if (index.isValid()) {
    QVariant v = index.data(RoleSearchMatchPositions);
    if (v.isValid() && !v.isNull())
      matchpos = klfVariantListToList<int>(v.toList());
  }
  // if there aren't any matched positions, let the base class take care
  if (matchpos.isEmpty()) {
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }

  // paint the match positions
  Q_ASSERT(index.isValid());

  QStyleOptionViewItemV4 opt = option;
  initStyleOption(&opt, index);
  
  const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
  QStyle *style = widget ? widget->style() : QApplication::style();
  style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

}




