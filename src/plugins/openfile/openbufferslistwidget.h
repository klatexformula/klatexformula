/***************************************************************************
 *   file plugins/openfile/openbufferslistwidget.h
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

#ifndef PLUGINS_OPENBUFFERSLISTWIDGET_H
#define PLUGINS_OPENBUFFERSLISTWIDGET_H

#include <QtCore>
#include <QtGui>

#include "openbuffer.h"


class OpenBuffersListWidget : public QListWidget
{
  Q_OBJECT
public:
  OpenBuffersListWidget(QWidget *parent)
    : QListWidget(parent)
  {
    setIconSize(QSize(150,50));

    pActiveBuffer = NULL;

    connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(ourSelectionChanged()));
  }

signals:
  void bufferSelected(OpenBuffer * buffer);

public slots:
  void setBufferList(QList<OpenBuffer*> buffers, OpenBuffer * activeBuffer)
  {
    int k;
    pItemsByBuffer.clear();
    clear();
    for (k = 0; k < buffers.size(); ++k) {
      QListWidgetItem *item = new QListWidgetItem(this);
      pItemsByBuffer[buffers[k]] = item;
      if (buffers[k] == activeBuffer) {
	//item->setSelected(true);
	setCurrentItem(item);
      }
      // setup the item
      refreshBuffer(buffers[k]);
    }
    pActiveBuffer = activeBuffer;
  }

  void refreshBuffer(OpenBuffer * buffer)
  {
    QListWidgetItem *item = pItemsByBuffer.value(buffer, NULL);
    if (item == NULL) {
      klfWarning("Can't find buffer "<<buffer) ;
      return;
    }
    item->setIcon(buffer->pixmap());
    QString s = buffer->file();
    if (buffer->ismodified())
      s += " *";
    item->setText(s);
  }

protected slots:

  void ourSelectionChanged()
  {
    QList<QListWidgetItem*> selected = selectedItems();
    if (selected.size() < 1) {
      emit bufferSelected(NULL) ;
      return;
    }
    if (selected.size() > 1) {
      klfWarning("WTF?!?!? more than one buffer selected?!");
    }
    QListWidgetItem *item = selected.first();
    QList<OpenBuffer*> buffers = pItemsByBuffer.keys(item);
    if (buffers.size() != 1) {
      klfWarning("WTF:?!?!? not exactly one buffer for item "<<item) ;
      emit bufferSelected(NULL);
      return;
    }
    emit bufferSelected(buffers.first());
  }

private:
  QHash<OpenBuffer*, QListWidgetItem*> pItemsByBuffer;
  OpenBuffer * pActiveBuffer;

};



#endif
