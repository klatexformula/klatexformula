/***************************************************************************
 *   file klfuiloader.cpp
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

#include "klfuiloader.h"
#include "klfuiloader_p.h"

#include "klflatexedit.h"

KLFUiLoader::~KLFUiLoader()
{
}

QWidget * KLFUiLoader::createWidget(const QString& className, QWidget * parent,
				    const QString & name)
{
  if (className == QLatin1String("KLFLatexEdit")) {
    QWidget * latexedit = new KLFLatexEdit(parent);
    latexedit->setObjectName(name);
    return latexedit;
  } else {
    return QUiLoader::createWidget(className, parent, name);
  }
}



QWidget * klfLoadUI(QIODevice *iodevice, QWidget * parent)
{
  KLFUiLoader loader;
  QWidget *widget = loader.load(iodevice, parent);
  KLF_ASSERT_NOT_NULL(widget, "Unable to load UI widget form!", return NULL; ) ;
  return widget;
}
