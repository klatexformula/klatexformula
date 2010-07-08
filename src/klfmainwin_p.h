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
#include <QLabel>

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



/** \internal */
class KLFMainWinPopup : public QLabel
{
  Q_OBJECT
public:
  KLFMainWinPopup(KLFMainWin *mainwin)
    : QLabel(mainwin, Qt::Window|Qt::FramelessWindowHint|Qt::CustomizeWindowHint|
	     Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint)
  {
    setObjectName("KLFMainWinPopup");
    setFocusPolicy(Qt::NoFocus);
    QWidget *frmMain = mainwin->findChild<QWidget*>("frmMain");
    if (frmMain)
      setGeometry(QRect(mainwin->geometry().topLeft()+QPoint(25,mainwin->height()*2/3),
			QSize(frmMain->width()+15, 0)));
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setProperty("klfTopLevelWidget", QVariant(true));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(mainwin->window()->styleSheet());
    setMargin(0);
    QFont f = font();
    f.setPointSize(QFontInfo(f).pointSize() - 1);
    setFont(f);
    setWordWrap(true);

    connect(this, SIGNAL(linkActivated(const QString&)),
	    this, SLOT(internalLinkActivated(const QString&)));
    connect(this, SIGNAL(linkActivated(const QUrl&)),
	    mainwin, SLOT(helpLinkAction(const QUrl&)));
  }

  virtual ~KLFMainWinPopup() { }

  bool hasMessages() { return (bool)msgKeys.size(); }

  QStringList messageKeys() { return msgKeys; }

signals:
  void linkActivated(const QUrl& url);

public slots:

  /** Displays the message \c msgText in the label. \c msgKey is arbitrary
   * and may be used to remove the message with removeMessage() later on.
   */
  void addMessage(const QString& msgKey, const QString& msgText)
  {
    klfDbg("adding message "<<msgKey<<" -> "<<msgText);
    msgKeys << msgKey;
    messages << ("<p>"+msgText+"</p>");
    updateText();
  }

  void removeMessage(const QString& msgKey)
  {
    int i = msgKeys.indexOf(msgKey);
    if (i < 0)
      return;
    msgKeys.removeAt(i);
    messages.removeAt(i);
    updateText();
  }

  void show()
  {
    QLabel::show();
    setStyleSheet(styleSheet());
  }

private slots:
  void internalLinkActivated(const QString& url)
  {
    emit linkActivated(QUrl::fromEncoded(url.toLatin1()));
  }

  void updateText()
  {
    setText("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
	    "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
	    "<html><head><meta name=\"qrichtext\" content=\"1\" />"
	    "<style type=\"text/css\">\n"
	    "body { margin: 2px 10px; }\n"
	    "p { white-space: pre-wrap; padding: 0px; margin: 0px 0px 2px 0px; }\n"
	    "a, a:link { text-decoration: underline; color: #0000af }\n"
	    "a:hover { color: #0000ff }\n"
	    "a:active { color: #ff0000 }\n"
	    "}\n"
	    "</style>"
	    "</head>"
	    "<body>\n"
	    + messages.join("\n") +
	    "<p style=\"margin-top: 5px\">"
	    "<a href=\"klfaction:/popup?acceptAll\">"+tr("Accept [<b>Alt-Enter</b>]")+"</a> | "+
	    " <a href=\"klfaction:/popup_close\">"+tr("Close [<b>Esc</b>]")+"</a> | " +
	    " <a href=\"klfaction:/popup?configDontShow\">"+tr("Don't Show Again")+"</a></p>"+
	    "</body></html>" );
    resize(width(), sizeHint().height());
  }

private:
  QStringList msgKeys;
  QStringList messages;
};



#endif
