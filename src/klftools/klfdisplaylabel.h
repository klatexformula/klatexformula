/***************************************************************************
 *   file klfdisplaylabel.h
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

#ifndef KLFDISPLAYLABEL_H
#define KLFDISPLAYLABEL_H

#include <QLabel>
#include <QPicture>
#include <QTemporaryFile>

#include <klfdefs.h>

/** \brief A label to display a LaTeX-formula-output-like image
 *
 * This widget displays an image, and sets another image as tooltip. It can also set itself
 * in "display error" mode, changing its look and displaying the error message as tooltip.
 *
 * It emits \ref labelDrag() whenever the user drags the display's contents, but only if
 * the display's property \c labelEnabled is TRUE (see setLabelEnabled()).
 *
 * As a gadget, it can add an alien glow to the images it displays.
 *
 * \note As of version 3.3.0, the \c labelFixedSize property dissapeared, it is no longer
 *   used. Use QWidget::setFixedSize() directly instead.
 */
class KLF_EXPORT KLFDisplayLabel : public QLabel
{
  Q_OBJECT

  Q_PROPERTY(bool enableToolTipPreview READ enableToolTipPreview WRITE setEnableToolTipPreview) ;

  Q_PROPERTY(QString bigPreviewText READ bigPreviewText) ;
  Q_PROPERTY(bool glowEffect READ glowEffect WRITE setGlowEffect) ;
  Q_PROPERTY(QColor glowEffectColor READ glowEffectColor WRITE setGlowEffectColor) ;
  Q_PROPERTY(int glowEffectRadius READ glowEffectRadius WRITE setGlowEffectRadius) ;
  Q_PROPERTY(bool labelEnabled READ labelEnabled WRITE setLabelEnabled) ;
public:
  KLFDisplayLabel(QWidget *parent);
  virtual ~KLFDisplayLabel();

  enum DisplayState { Clear = 0, Ok, Error };
  virtual DisplayState currentDisplayState() const { return pDisplayState; }

  /** \brief maximum pixmap size we can display
   *
   * For now, it just returns size().
   *
   * \todo subtract margins/frame border width/etc.
   * */
  virtual QSize labelSize() const { return size(); }

  virtual bool enableToolTipPreview() const { return pEnableToolTipPreview; }

  virtual QString bigPreviewText() const { return _bigPreviewText; }

  inline bool glowEffect() const { return pGE; }
  inline QColor glowEffectColor() const { return pGEcolor; }
  inline int glowEffectRadius() const { return pGEradius; }

  inline bool labelEnabled() const { return pLabelEnabled; }

signals:
  void labelDrag();

public slots:
  virtual void setEnableToolTipPreview(bool enable) { pEnableToolTipPreview = enable; display_state(pDisplayState); }

  virtual void displayClear();
  virtual void display(QImage displayimg, QImage tooltipimage, bool labelenabled = true);
  virtual void displayError(bool labelenabled = false)
  { displayError(QString(), labelenabled); }
  virtual void displayError(const QString& errorMessage, bool labelenabled = false);

  void setGlowEffect(bool on) { pGE = on; display_state(pDisplayState); }
  void setGlowEffectColor(const QColor& color) { pGEcolor = color; display_state(pDisplayState); }
  void setGlowEffectRadius(int r) { pGEradius = r; display_state(pDisplayState); }

  void setLabelEnabled(bool enabled) { pLabelEnabled = enabled; display_state(pDisplayState); }

protected:
  virtual void mouseMoveEvent(QMouseEvent *e);

private:

  QPicture calc_display_picture();
  void display_state(DisplayState state);

  DisplayState pDisplayState;
  QString pDisplayError;
  QImage pDisplayImage;
  QImage pDisplayTooltip;

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

  bool pLabelEnabled;
};







#endif
