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


/** \internal */
class KLFLibSqliteOpenWidget : public QWidget, protected Ui::KLFLibSqliteOpenWidget
{
  Q_OBJECT
public:
  KLFLibSqliteOpenWidget(QWidget *parent) : QWidget(parent)
  {
    setupUi(this);
    setProperty("readyToOpen", false);
  }
  virtual ~KLFLibSqliteOpenWidget() { }

  virtual void setUrl(const QUrl& url) {
    txtFile->setText(url.path());
  }
  virtual QUrl url() const {
    QUrl url = QUrl::fromLocalFile(txtFile->text());
    url.setScheme("klf+sqlite");
    return url;
  }

signals:
  void readyToOpen(bool ready);

protected slots:
  virtual void on_btnBrowse_clicked()
  {
    QString filter = tr("KLatexFormula Library Resource Files (*.klf.db);;All Files (*)");
    static QString selectedFilter;
    QString name = QFileDialog::getOpenFileName(this, tr("Select Library Resource File"),
						QDir::homePath(), filter, &selectedFilter);
    if ( ! name.isEmpty() )
      txtFile->setText(name);
  }
  virtual void on_txtFile_textChanged(const QString& text)
  {
    emit readyToOpen(QFileInfo(text).isFile());
  }

};

// ---

/** \internal */
class KLFLibSqliteCreateWidget : public KLFLibSqliteOpenWidget
{
  Q_OBJECT
public:
  KLFLibSqliteCreateWidget(QWidget *parent) : KLFLibSqliteOpenWidget(parent)
  {
    /// \todo ...TODO.....: Add option to change main table name
    pConfirmedOverwrite = false;
    setProperty("readyToCreate", false);
  }
  virtual ~KLFLibSqliteCreateWidget() { }

  QString fileName() const {
    return txtFile->text();
  }

  bool confirmedOverwrite() const { return pConfirmedOverwrite; }

signals:
  void readyToCreate(bool ready);

protected slots:
  virtual void on_btnBrowse_clicked()
  {
    QString filter = tr("KLatexFormula Library Resource Files (*.klf.db);;All Files (*)");
    static QString selectedFilter;
    QString name = QFileDialog::getSaveFileName(this, tr("Select Library Resource File"),
						QDir::homePath(), filter, &selectedFilter);
    if ( ! name.isEmpty() ) {
      pConfirmedOverwrite = true;
      txtFile->setText(name);
    }
  }
  virtual void on_txtFile_textChanged(const QString& text)
  {
    pConfirmedOverwrite = false;
    emit readyToCreate(QFileInfo(text).absoluteDir().exists());
  }

protected:
  bool pConfirmedOverwrite;
};


#endif
