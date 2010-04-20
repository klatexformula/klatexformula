/***************************************************************************
 *   file klflibdbengine_p.h
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
 * library routines defined in klflibdbengine.cpp */


#ifndef KLFLIBDBENGINE_P_H
#define KLFLIBDBENGINE_P_H


#include <QUrl>
#include <QWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QPushButton>

#include <ui_klflibsqliteopenwidget.h>

#include "klflib.h"
#include "klflibdbengine.h"


/** \internal */
class KLFLibSqliteOpenWidget : public QWidget, protected Ui::KLFLibSqliteOpenWidget
{
  Q_OBJECT
public:
  KLFLibSqliteOpenWidget(QWidget *parent) : QWidget(parent)
  {
    setupUi(this);
    setProperty("readyToOpen", false);
    updateTables();
  }
  virtual ~KLFLibSqliteOpenWidget() { }

  virtual void setUrl(const QUrl& url) {
    txtFile->setText(url.path());
  }
  virtual QUrl url() const {
    QUrl url = QUrl::fromLocalFile(txtFile->text());
    url.setScheme("klf+sqlite");
    int tindex = cbxTable->currentIndex();
    if (tindex > 0) {
      QString datatablename = cbxTable->currentText();
      url.addQueryItem("dataTableName", datatablename);
    } else {
      url.addQueryItem("dataTableName", "klfentries");
    }
    return url;
  }

signals:
  void readyToOpen(bool ready);

protected:
  virtual bool isReadyToOpen(const QString& text = QString()) const
  {
    QString t = text.isNull() ? txtFile->text() : text;
    return QFileInfo(t).isFile();
  }


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
    bool ready = isReadyToOpen(text);
    if (ready != property("readyToOpen").toBool()) {
      // ready to open state changes
      setProperty("readyToOpen", ready);
      emit readyToOpen(ready);
    }
    updateTables();
  }
  virtual void updateTables()
  {
    // clear combo box
    while (cbxTable->count() > 1)
      cbxTable->removeItem(1);
    if (!property("readyToOpen").toBool()) {
      cbxTable->setEnabled(false);
      return;
    }
    cbxTable->setEnabled(true);
    // get tables
    QStringList datatables = KLFLibDBEngine::getDataTableNames(url());
    // and populate combo
    int k;
    for (k = 0; k < datatables.size(); ++k) {
      cbxTable->addItem(datatables[k], QVariant::fromValue(datatables[k]));
    }
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

    // can only create a "klfentries" table
    lblTable->hide();
    cbxTable->hide();
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


/** \internal */
class KLFLibDBEnginePropertyChangeNotifier : public QObject
{
  Q_OBJECT
public:
  KLFLibDBEnginePropertyChangeNotifier(const QString& dbname, QObject *parent)
    : QObject(parent), pDBName(dbname), pRef(0) { }
  virtual ~KLFLibDBEnginePropertyChangeNotifier() { }

  void ref() { ++pRef; }
  //! Returns TRUE if the object needs to be deleted.
  bool deRef() { return !--pRef; }

  void notifyResourcePropertyChanged(int propId)
  {
    emit resourcePropertyChanged(propId);
  }

signals:
  void resourcePropertyChanged(int propId);

private:
  QString pDBName;
  int pRef;
};




#endif
