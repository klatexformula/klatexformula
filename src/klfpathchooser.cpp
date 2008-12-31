/***************************************************************************
 *   file klfpathchooser.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2008 by Philippe Faist
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

#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QFileDialog>

#include "klfpathchooser.h"


KLFPathChooser::KLFPathChooser(QWidget *parent)
  : QFrame(parent), _mode(0), _caption(), _filter()
{
  setFrameShape(QFrame::Box);
  setFrameShadow(QFrame::Raised);

  QHBoxLayout *lyt = new QHBoxLayout(this);
  lyt->setContentsMargins(2,2,2,2);
  lyt->setSpacing(2);
  txtPath = new QLineEdit(this);
  lyt->addWidget(txtPath);
  btnBrowse = new QPushButton(tr("Browse"), this);
  lyt->addWidget(btnBrowse);

  connect(btnBrowse, SIGNAL(clicked()), this, SLOT(requestBrowse()));
}

KLFPathChooser::~KLFPathChooser()
{
}


QString KLFPathChooser::path() const
{
  return txtPath->text();
}

void KLFPathChooser::setPath(const QString& path)
{
  txtPath->setText(path);
}

void KLFPathChooser::requestBrowse()
{
  QString s;
  if (_mode == 1) {
    // save
    s = QFileDialog::getSaveFileName(this, _caption, txtPath->text(), _filter, &_selectedfilter);
  } else if (_mode == 2) {
    s = QFileDialog::getExistingDirectory(this, _caption, txtPath->text(), 0/*options*/);
  } else {
    // open
    s = QFileDialog::getOpenFileName(this, _caption, txtPath->text(), _filter, &_selectedfilter);
  }
  if ( ! s.isEmpty() ) {
    setPath(s);
  }
}


void KLFPathChooser::setCaption(const QString& caption)
{
  _caption = caption;
}

void KLFPathChooser::setMode(int mode)
{
  _mode = mode;
}

void KLFPathChooser::setFilter(const QString& filter)
{
  _filter = filter;
}
