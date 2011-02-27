/***************************************************************************
 *   file klfpathchooser.cpp
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

#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDirModel>
#include <QCompleter>

#include "klfpathchooser.h"


KLFPathChooser::KLFPathChooser(QWidget *parent)
  : QFrame(parent), _mode(0), _caption(), _filter(), _dlgconfirmoverwrite(true),
    _pathFromDialog(false)
{
  //  setFrameShape(QFrame::Box);
  //  setFrameShadow(QFrame::Raised);
  setFrameStyle(QFrame::NoFrame|QFrame::Plain);

  QHBoxLayout *lyt = new QHBoxLayout(this);
  //  lyt->setContentsMargins(2,2,2,2);
  lyt->setContentsMargins(0,0,0,0);
  lyt->setSpacing(2);
  txtPath = new QLineEdit(this);
  lyt->addWidget(txtPath);
  btnBrowse = new QPushButton(tr("Browse"), this);
  btnBrowse->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  lyt->addWidget(btnBrowse);

  // set a filename completer for txtPath
  QDirModel *dirModel = new QDirModel(QStringList(),
				      QDir::AllEntries|QDir::AllDirs|QDir::NoDotAndDotDot,
				      QDir::DirsFirst|QDir::IgnoreCase, this);
  QCompleter *fileNameCompleter = new QCompleter(this);
  fileNameCompleter->setModel(dirModel);
  txtPath->setCompleter(fileNameCompleter);

  // connect signals
  connect(txtPath, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
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
  _pathFromDialog = false;
}

void KLFPathChooser::requestBrowse()
{
  QFileDialog::Options options = 0;
  if (_mode == 1 && !_dlgconfirmoverwrite)
    options |= QFileDialog::DontConfirmOverwrite;

  QString path;
  if (!txtPath->text().isEmpty())
    path = txtPath->text();
  else
    path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

  QString s;
  if (_mode == 1) {
    // save
    s = QFileDialog::getSaveFileName(this, _caption, path, _filter, &_selectedfilter, options);
  } else if (_mode == 2) {
    s = QFileDialog::getExistingDirectory(this, _caption, path, 0/*options*/);
  } else {
    // open
    s = QFileDialog::getOpenFileName(this, _caption, path, _filter, &_selectedfilter);
  }
  if ( ! s.isEmpty() ) {
    setPath(s);
    if (_mode == 1 && _dlgconfirmoverwrite)
      _pathFromDialog = true;
    emit fileDialogPathChosen(s);
  }
}


void KLFPathChooser::setCaption(const QString& caption)
{
  _caption = caption;
}

void KLFPathChooser::setMode(int mode)
{
  _mode = mode;
  _pathFromDialog = false; // no overwrite confirmed yet
}

void KLFPathChooser::setFilter(const QString& filter)
{
  _filter = filter;
}

void KLFPathChooser::slotTextChanged()
{
  _pathFromDialog = false;
}

