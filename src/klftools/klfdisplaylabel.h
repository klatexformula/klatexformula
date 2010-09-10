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

#include <klfdefs.h>

/** \brief A label to display a LaTeX-formula-output-like image
 */
class KLF_EXPORT KLFDisplayLabel : public QLabel
{
  Q_OBJECT

  Q_PROPERTY(QSize labelFixedSize READ labelFixedSize WRITE setLabelFixedSize) ;
  Q_PROPERTY(bool enableToolTipPreview READ enableToolTipPreview WRITE setEnableToolTipPreview) ;

  Q_PROPERTY(QString bigPreviewText READ bigPreviewText) ;
  Q_PROPERTY(bool glowEffect READ glowEffect WRITE setGlowEffect) ;
  Q_PROPERTY(QColor glowEffectColor READ glowEffectColor WRITE setGlowEffectColor) ;
  Q_PROPERTY(int glowEffectRadius READ glowEffectRadius WRITE setGlowEffectRadius) ;
public:
  KLFDisplayLabel(QWidget *parent);
  virtual ~KLFDisplayLabel();

  virtual QSize labelFixedSize() const { return pLabelFixedSize; }
  virtual bool enableToolTipPreview() const { return pEnableToolTipPreview; }

  virtual QString bigPreviewText() const { return _bigPreviewText; }

  inline bool glowEffect() const { return pGE; }
  inline QColor glowEffectColor() const { return pGEcolor; }
  inline int glowEffectRadius() const { return pGEradius; }

signals:
  void labelDrag();

public slots:
  virtual void setLabelFixedSize(const QSize& size);
  virtual void setEnableToolTipPreview(bool enable) { pEnableToolTipPreview = enable; }

  virtual void displayClear();
  virtual void display(QImage displayimg, QImage tooltipimage, bool labelenabled = true);
  virtual void displayError(bool labelenabled = false);

  /** \note takes effect only upon next call of display() */
  void setGlowEffect(bool on) { pGE = on; }
  /** \note takes effect only upon next call of display() */
  void setGlowEffectColor(const QColor& color) { pGEcolor = color; }
  /** \note takes effect only upon next call of display() */
  void setGlowEffectRadius(int r) { pGEradius = r; }

protected:
  virtual void mouseMoveEvent(QMouseEvent *e);

private:

  QSize pLabelFixedSize;
  bool pEnableToolTipPreview;
  QTemporaryFile *mToolTipFile;

  QPalette pDefaultPalette;
  QPalette pErrorPalette;

  QString _bigPreviewText;

  bool pGE;
  QColor pGEcolor;
  int pGEradius;

  void set_error(bool error_on);
};







#endif
