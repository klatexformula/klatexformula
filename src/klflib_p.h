/***************************************************************************
 *   file klflib_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflib.cpp */

#ifndef KLFLIB_P_H
#define KLFLIB_P_H

#include <QUrl>
#include <QWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QPushButton>

#include <ui_klflibsqliteopenwidget.h>


class KLFLibSqliteOpenWidget : public QWidget, private Ui::KLFLibSqliteOpenWidget
{
  Q_OBJECT
public:
  KLFLibSqliteOpenWidget(QWidget *parent) : QWidget(parent)
  {
    setupUi(this);
  }
  virtual ~KLFLibSqliteOpenWidget() { }

  void setUrl(const QUrl& url) {
    txtFile->setText(url.path());
  }
  QUrl url() const { return QUrl::fromLocalFile(txtFile->text()); }

signals:
  void readyToOpen(bool ready);

private slots:
  void btnBrowse_clicked() {
    QString name = QFileDialog::getOpenFileName(this);
    if ( ! name.isEmpty() )
      txtFile->setText(name);
  }
  void txtFile_textChanged(const QString& text)
  {
    emit readyToOpen(QFile::exists(text));
  }
};



#endif
