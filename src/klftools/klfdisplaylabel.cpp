/***************************************************************************
 *   file klfdisplaylabel.cpp
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

#include <QLabel>
#include <QDir>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QVariant>
#include <QPainter>

#include <klfguiutil.h>
#include "klfdisplaylabel.h"


KLFDisplayLabel::KLFDisplayLabel(QWidget *parent)
  : QLabel(parent), pEnableToolTipPreview(true), mToolTipFile(NULL)
{
  setText(QString());
  //  setLabelFixedSize(QSize(120,80));

  setAlignment(Qt::AlignCenter);

  setScaledContents(true);

  pDefaultPalette = palette();
  pErrorPalette = pDefaultPalette;

  pDefaultPalette.setColor(QPalette::Window, QColor(255, 255, 255, 0)); // fully transparent
  pErrorPalette.setColor(QPalette::Window, QColor(255, 0, 0, 60)); // red color, semi-transparent

  pGE = false;
  pGEcolor = QColor(128, 255, 128, 8);
  pGEradius = 4;
}

KLFDisplayLabel::~KLFDisplayLabel()
{
  if (mToolTipFile)
    delete mToolTipFile;
}

/*
void KLFDisplayLabel::setLabelFixedSize(const QSize& size)
{
  pLabelFixedSize = size;
  setMinimumSize(size);
  setFixedSize(size);
}
*/

void KLFDisplayLabel::displayClear()
{
  display_state(Clear);
  //  setEnabled(false);
  pLabelEnabled = false;
}

void KLFDisplayLabel::display(QImage displayimg, QImage tooltipimage, bool labelenabled)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  pDisplayImage = displayimg;
  pDisplayTooltip = tooltipimage;

  pLabelEnabled = labelenabled;
  display_state(Ok);
}

void KLFDisplayLabel::displayError(const QString& errorMessage, bool labelenabled)
{
  pDisplayError = errorMessage;

  pLabelEnabled = labelenabled;
  display_state(Error);
}


QPicture KLFDisplayLabel::calc_display_picture()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
 
  QImage img = pDisplayImage;
  QPixmap pix;
  QSize mysize = (QSizeF(size()) * devicePixelRatioF()).toSize();
  klfDbg("widget size()="<<size()<<", mysize="<<mysize) ;
  if (/*labelenabled && */ pGE) {
    int r = pGEradius * devicePixelRatioF();
    QSize msz = QSize(2*r, 2*r);
    if (img.width()+msz.width() > width() || img.height()+msz.height() > height())
      img = pDisplayImage.scaled(mysize-msz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pix = QPixmap(img.size()+msz);
    pix.fill(QColor(0,0,0,0));
    QPainter painter(&pix);
    painter.translate(QPoint(r, r));
    klfDrawGlowedImage(&painter, img, pGEcolor, r);
  } else {
    if (img.width() > mysize.width() || img.height() > mysize.height()) {
      img = pDisplayImage.scaled(mysize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    pix = QPixmap::fromImage(img);
  }
  pix.setDevicePixelRatio(1.0);//devicePixelRatioF());

  QPicture labelpic;
  labelpic.setBoundingRect(QRect(QPoint(0,0), mysize));
  QPainter pp(&labelpic);
  if (!pLabelEnabled) {
    pp.setOpacity(0.5f);
  }
  pp.drawPixmap(QPoint((mysize.width()-pix.width())/2, (mysize.height()-pix.height())/2),
		pix);
  //  // desaturate/grayify the pixmap if we are label-disabled
  //  if (!pLabelEnabled) {
  //    pp.fillRect(QRect(QPoint(0,0), mysize), QColor(255,255,255, 90));
  //  }
  return labelpic;
}

void KLFDisplayLabel::display_state(DisplayState state)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  pDisplayState = state;
  if (state == Clear) {
    setPicture(QPicture());
    setText(QString());
    set_error(false);
  }
  if (state == Error) {
    set_error(true);
    setToolTip(pDisplayError);
    _bigPreviewText = pDisplayError;
  }
  if (state == Ok) {
    QPicture labelpic = calc_display_picture();
    setPicture(labelpic);

    // un-set any error
    set_error(false);

    if (mToolTipFile) {
      delete mToolTipFile;
      mToolTipFile = 0;
    }
    // no big preview by default
    _bigPreviewText = "";
    // but if one is given then prepare it (prepare it even if "enableToolTipPreview" is false,
    // because we will need it for the "showBigPreview" button)
    if ( ! pDisplayTooltip.isNull() ) {
      QString tempdir = QDir::tempPath();
      mToolTipFile = new QTemporaryFile(tempdir+"/klf_tooltip_XXXXXX.png", this);
      if ( ! mToolTipFile->open() ) {
	qWarning("WARNING: Failed open for ToolTip Temp Image!\n%s\n",
		 qPrintable(mToolTipFile->fileTemplate()));
	delete mToolTipFile;
	mToolTipFile = 0;
      } else {
	mToolTipFile->setAutoRemove(true);
	bool res = pDisplayTooltip.save(mToolTipFile, "PNG");
	if ( ! res ) {
	  QMessageBox::critical(this, tr("Error"), tr("Failed write to ToolTip Temp Image file %1!")
				.arg(mToolTipFile->fileName()));
	  qWarning("WARNING: Failed write to Tooltip temp image to temporary file `%s' !\n",
		   qPrintable(mToolTipFile->fileTemplate()));
	  delete mToolTipFile;
	  mToolTipFile = 0;
	} else {
	  _bigPreviewText = QString("<img src=\"%1\" width=\"%2\" height=\"%3\" style=\"width:%2px; height:%3px;\">")
            .arg(mToolTipFile->fileName())
            .arg((int)(pDisplayTooltip.width() / devicePixelRatioF()))
            .arg((int)(pDisplayTooltip.height() / devicePixelRatioF()));
          klfDbg("big preview html = " << _bigPreviewText) ;
	}
      }
    }
    if (pEnableToolTipPreview) {
      setToolTip(QString("<p style=\"padding: 8px 8px 8px 8px;\">%1</p>").arg(_bigPreviewText));
    } else {
      setToolTip(QString(""));
    }
  }
}

void KLFDisplayLabel::set_error(bool error_on)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  setProperty("realTimeLatexError", QVariant(error_on));
  QPalette *p;
  if (error_on) {
    p = &pErrorPalette;
  } else {
    p = &pDefaultPalette;
  }
  setAutoFillBackground(true);
  setStyleSheet(styleSheet()); // force style sheet refresh
  setPalette(*p);
}


void KLFDisplayLabel::mouseMoveEvent(QMouseEvent */*e*/)
{
  if (pLabelEnabled)
    emit labelDrag();
}
