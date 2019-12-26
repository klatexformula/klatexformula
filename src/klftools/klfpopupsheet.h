/***************************************************************************
 *   file klfpopupsheet.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2019 by Philippe Faist
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

#ifndef KLFPOPUPSHEET_H
#define KLFPOPUPSHEET_H

#include <klfdefs.h>

#include <QWidget>

class QAbstractButton;
class QPaintEvent;
struct KLFPopupSheetPrivate;

class KLFPopupSheet : public QWidget
{
  Q_OBJECT
public:
  KLFPopupSheet(QWidget * parent);
  virtual ~KLFPopupSheet();

  void associateWithButton(QAbstractButton * button);

  void setCentralWidget(QWidget * w);

signals:
  void popupVisible(bool on);
  void popupShown();
  void popupHidden();

public slots:
  void showPopup(const QPoint & pos);
  void hidePopup();

protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);

private:
  KLF_DECLARE_PRIVATE(KLFPopupSheet) ;
};


#endif
