/***************************************************************************
 *   file klfdisplaylabel.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#ifndef KLFDISPLAYLABEL_H
#define KLFDISPLAYLABEL_H

#include <QLabel>
#include <QTemporaryFile>

/** \brief A label to display a LaTeX-formula-output-like image
 */
class KLFDisplayLabel : public QLabel
{
  Q_OBJECT

  Q_PROPERTY(QSize labelFixedSize READ labelFixedSize WRITE setLabelFixedSize)
  Q_PROPERTY(QString bigPreviewText READ bigPreviewText)
public:
  KLFDisplayLabel(QWidget *parent);
  virtual ~KLFDisplayLabel();

  QSize labelFixedSize() const { return pLabelFixedSize; }

  QString bigPreviewText() const { return _bigPreviewText; }

signals:
  void labelDrag();

public slots:
  void setLabelFixedSize(const QSize& size);

  void displayClear();
  void display(QImage displayimg, QImage tooltipimage, bool labelenabled = true);
  void displayError(bool labelenabled = false);

protected:

  void mouseMoveEvent(QMouseEvent *e);

private:
  QSize pLabelFixedSize;
  QTemporaryFile *mTooltipFile;

  QPalette pDefaultPalette;
  QPalette pErrorPalette;

  QString _bigPreviewText;

  void set_error(bool error_on);
};







#endif
