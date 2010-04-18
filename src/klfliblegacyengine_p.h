/***************************************************************************
 *   file klfliblegacyengine_p.h
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
 * library routines defined in klfliblegacyengine.cpp */

#ifndef KLFLIBLEGACYENGINE_P_H
#define KLFLIBLEGACYENGINE_P_H


#include <QUrl>
#include <QWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QPushButton>

#include <ui_klflibsqliteopenwidget.h>

#include "klflib.h"
#include "klfliblegacyengine.h"




/** \internal */
class KLFLibLegacyOpenWidget : public QWidget, protected Ui::KLFLibSqliteOpenWidget
{
  Q_OBJECT
public:
  KLFLibLegacyOpenWidget(QWidget *parent) : QWidget(parent)
  {
    setupUi(this);
    setProperty("readyToOpen", false);
    updateLegacyResources();
    lblTable->setText("Resource To Open:");
  }
  virtual ~KLFLibLegacyOpenWidget() { }

  virtual void setUrl(const QUrl& url) {
    txtFile->setText(url.path());
  }
  virtual QUrl url() const {
    QUrl url = QUrl::fromLocalFile(txtFile->text());
    url.setScheme("klf+legacy");
    int tindex = cbxTable->currentIndex();
    if (tindex > 0) {
      QString lresname = cbxTable->itemData(tindex).toString();
      url.addQueryItem("legacyResourceName", lresname);
    } else {
      url.addQueryItem("legacyResourceName", "");
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
    QString filter = tr("KLatexFormula 3.1 Library Export Files (*.klf);;All Files (*)");
    static QString selectedFilter;
    QString name = QFileDialog::getOpenFileName(this, tr("Select Library Export File"),
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
    updateLegacyResources();
  }
  virtual void updateLegacyResources()
  {
    // clear combo box
    while (cbxTable->count() > 1)
      cbxTable->removeItem(1);
    if (!property("readyToOpen").toBool()) {
      cbxTable->setEnabled(false);
      return;
    }
    cbxTable->setEnabled(true);
    // get openable resource names
    QStringList lresources = KLFLibLegacyEngine::getLegacyResourceNames(url());
    // and populate combo
    int k;
    for (k = 0; k < lresources.size(); ++k) {
      cbxTable->addItem(lresources[k], QVariant::fromValue(lresources[k]));
    }
  }

};

// ---

/** \internal */
class KLFLibLegacyCreateWidget : public KLFLibLegacyOpenWidget
{
  Q_OBJECT
public:
  KLFLibLegacyCreateWidget(QWidget *parent) : KLFLibLegacyOpenWidget(parent)
  {
    /// \todo ...TODO.....: Add option to change main table name
    pConfirmedOverwrite = false;
    setProperty("readyToCreate", false);

    // can only create a "Resource" table
    lblTable->hide();
    cbxTable->hide();
  }
  virtual ~KLFLibLegacyCreateWidget() { }

  QString fileName() const {
    return txtFile->text();
  }

  bool confirmedOverwrite() const { return pConfirmedOverwrite; }

signals:
  void readyToCreate(bool ready);

protected slots:
  virtual void on_btnBrowse_clicked()
  {
    QString filter = tr("KLatexFormula 3.1 Library Export Files (*.klf);;All Files (*)");
    static QString selectedFilter;
    QString name = QFileDialog::getSaveFileName(this, tr("Select Library Export File"),
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
