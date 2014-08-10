/***************************************************************************
 *   file klfprogerr.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
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

#include <QRegExp>

#include "klfprogerr.h"

#include "ui_klfprogerr.h"


KLFProgErr::KLFProgErr(QWidget *parent, QString errtext) : QDialog(parent)
{
  u = new Ui::KLFProgErr;
  u->setupUi(this);
  setObjectName("KLFProgErr");

  setWindowFlags(Qt::Sheet);

  setWindowModality(Qt::WindowModal);

  u->txtError->setWordWrapMode(QTextOption::WrapAnywhere);
  u->txtError->setText(errtext);
}

QTextEdit * KLFProgErr::textEditWidget()
{
  return u->txtError;
}


KLFProgErr::~KLFProgErr()
{
  delete u;
}

void KLFProgErr::showEvent(QShowEvent */*e*/)
{
}

void KLFProgErr::showError(QWidget *parent, QString errtext)
{
  KLFProgErr dlg(parent, errtext);
  dlg.exec();
}



// ------------------------------------------------------------------------


QString KLFProgErr::extractLatexError(const QString& str)
{
  // if it was LaTeX, attempt to parse the error...
  QRegExp latexerr("\\n(\\!.*)\\n\\n");
  if (latexerr.indexIn(str)) {
    QString s = latexerr.cap(1);
    s.replace(QRegExp("^([^\\n]+)"), "<b>\\1</b>"); // make first line boldface
    return "<pre>"+s+"</pre>";
  }
  return str;
}

