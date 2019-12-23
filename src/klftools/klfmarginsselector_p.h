/***************************************************************************
 *   file klfmarginsselector_p.h
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

#ifndef KLFMARGINSSELECTOR_P_H
#define KLFMARGINSSELECTOR_P_H


#include "klfmarginsselector.h"

#include <ui_klfmarginseditor.h>

class KLFMarginsEditor : public QWidget
{
  Q_OBJECT
public:
  KLFMarginsEditor(QWidget * parent, bool use_dialog = false);
  ~KLFMarginsEditor();

  Ui::KLFMarginsEditor * u;

  void setVisible(bool on);

signals:
  void editorUpdatedMargins(); //qreal t, qreal r, qreal b, qreal l);
  void visibilityChanged(bool);

private slots:
  void emit_updated_margins();
  void show_unit_changed(double, QString);

private:
  bool ignore_valuechange_event;
  bool use_dialog;
};




struct KLFMarginsSelectorPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFMarginsSelector, QObject) { }

  KLFMarginsEditor * mMarginsEditor;

public slots:
  void editorButtonToggled(bool);
  void editorVisibilityChanged(bool);

  void updateMarginsDisplay();
};


#endif
