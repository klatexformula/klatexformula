/***************************************************************************
 *   file $FILENAME$
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

#ifndef KLFMAINWIN_P_H
#define KLFMAINWIN_P_H

#include <QDialog>
#include <QFile>
#include <QString>
#include <QByteArray>

#include "klfmain.h"

#include "klfmainwin.h"

#include <ui_klfaboutdialog.h>
#include <ui_klfwhatsnewdialog.h>


class KLFHelpDialogCommon
{
public:
  KLFHelpDialogCommon(const QString& baseFName) : pBaseFName(baseFName) { }

  virtual void addWhatsNewText(const QString& htmlSnipplet)
  {
    pHtmlExtraSnipplets << htmlSnipplet;
  }

  virtual QString getFullHtml()
  {
    QString html;
    QString fn;
    QString loc = klfconfig.UI.locale;
    QStringList suffixes = QStringList()<<"_"+loc<<"_"+loc.section('_',0,0)<<QString("");
    int k;
    while ( k < suffixes.size() &&
	    ! QFile::exists(fn = QString(":/data/%1%2.html").arg(pBaseFName, suffixes[k])) ) {
      qDebug()<<KLF_FUNC_NAME<<": tried fn="<<fn;
      ++k;
    }
    if (k >= suffixes.size()) {
      qWarning()<<KLF_FUNC_NAME<<": Can't find good file! last try="<<fn;
      return QString();
    }
    QFile f(fn);
    f.open(QIODevice::ReadOnly);
    html = QString::fromUtf8(f.readAll());

    qDebug()<<KLF_FUNC_NAME<<"(): read from file="<<fn<<" the HTML code=\n"<<html;

    const QString marker = QLatin1String("<!--KLF_MARKER_INSERT_SPOT-->");
    for (k = 0; k < pHtmlExtraSnipplets.size(); ++k)
      html.replace(marker, pHtmlExtraSnipplets[k]+"\n"+marker);

    qDebug()<<KLF_FUNC_NAME<<": new HTML is:\n"<<html;

    return html;
  }

protected:
  QString pBaseFName;
  QStringList pHtmlExtraSnipplets;
};

class KLFAboutDialog : public QDialog, public KLFHelpDialogCommon
{
  Q_OBJECT
signals:
  void linkActivated(const QUrl&);
public:
  KLFAboutDialog(QWidget *parent) : QDialog(parent), KLFHelpDialogCommon("about")
  {
    u = new Ui::KLFAboutDialog;
    u->setupUi(this);

    connect(u->txtDisplay, SIGNAL(anchorClicked(const QUrl&)),
	    this, SIGNAL(linkActivated(const QUrl&)));
  }
  virtual ~KLFAboutDialog() { }

  virtual void show()
  {
    u->txtDisplay->setHtml(getFullHtml());
    QDialog::show();
    // refresh our style sheet
    setStyleSheet(styleSheet());
  }

private:
  Ui::KLFAboutDialog *u;
};

class KLFWhatsNewDialog : public QDialog, public KLFHelpDialogCommon
{
  Q_OBJECT
signals:
  void linkActivated(const QUrl&);
public:
  KLFWhatsNewDialog(QWidget *parent)
    : QDialog(parent),
      KLFHelpDialogCommon(QString("whats-new-%1.%2").arg(version_maj).arg(version_min))
  {
    u = new Ui::KLFWhatsNewDialog;
    u->setupUi(this);

    connect(u->txtDisplay, SIGNAL(anchorClicked(const QUrl&)),
	    this, SIGNAL(linkActivated(const QUrl&)));
  }
  virtual ~KLFWhatsNewDialog() { }

  virtual void show()
  {
    u->txtDisplay->setHtml(getFullHtml());
    QDialog::show();
    // refresh our style sheet
    setStyleSheet(styleSheet());
  }

private:
  Ui::KLFWhatsNewDialog *u;
};



#endif
