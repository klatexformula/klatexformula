/***************************************************************************
 *   file klfmainwin_p.h
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


/** \file
 * This file contains internal definitions for file klfmainwin.cpp.
 * \internal
 */

#include <QDialog>
#include <QFile>
#include <QString>
#include <QByteArray>

#include "klfmain.h"

#include "klfmainwin.h"

#include <ui_klfaboutdialog.h>
#include <ui_klfwhatsnewdialog.h>

/** \internal */
class KLFHelpDialogCommon
{
public:
  KLFHelpDialogCommon(const QString& baseFName) : pBaseFName(baseFName) { }

  virtual void addExtraText(const QString& htmlSnipplet)
  {
    pHtmlExtraSnipplets << htmlSnipplet;
  }

  virtual QString getFullHtml()
  {
    QString html;
    QString fn = klfFindTranslatedDataFile(":/data/"+pBaseFName, ".html");

    QFile f(fn);
    f.open(QIODevice::ReadOnly);
    html = QString::fromUtf8(f.readAll());

    klfDbg( "read from file="<<fn<<" the HTML code=\n"<<html ) ;

    const QString marker = QLatin1String("<!--KLF_MARKER_INSERT_SPOT-->");
    int k;
    for (k = 0; k < pHtmlExtraSnipplets.size(); ++k)
      html.replace(marker, pHtmlExtraSnipplets[k]+"\n"+marker);

    klfDbg( "new HTML is:\n"<<html ) ;

    return html;
  }

protected:
  QString pBaseFName;
  QStringList pHtmlExtraSnipplets;
};


/** \internal */
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


/** \internal */
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
