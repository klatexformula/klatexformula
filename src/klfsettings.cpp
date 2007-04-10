/***************************************************************************
 *   file klfsettings.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#include <qdialog.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <kurlrequester.h>
#include <kcolorcombo.h>
#include <kstandarddirs.h>
#include <kfontrequester.h>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klfsettings.h"


KLFSettings::KLFSettings(KLFBackend::klfSettings *p, KLFLatexSyntaxHighlighter *sh, KLFMainWin* parent)
    : KLFSettingsUI(parent, 0, false, 0)
{
  _ptr = p;
  _synthighlighterptr = sh;
  _mainwin = parent;

  kurlTempDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
  kurlLatex->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
  kurlDvips->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
  kurlGs->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
  kurlEpstopdf->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);

  kurlTempDir->setURL(_ptr->tempdir);
  kurlLatex->setURL(_ptr->latexexec);
  kurlDvips->setURL(_ptr->dvipsexec);
  kurlGs->setURL(_ptr->gsexec);
  kurlEpstopdf->setURL(_ptr->epstopdfexec);
  chkEpstopdf->setChecked( ! _ptr->epstopdfexec.isEmpty() );
  spnLBorderOffset->setValue( _ptr->lborderoffset );
  spnTBorderOffset->setValue( _ptr->tborderoffset );
  spnRBorderOffset->setValue( _ptr->rborderoffset );
  spnBBorderOffset->setValue( _ptr->bborderoffset );

  chkSHEnabled->setChecked(_synthighlighterptr->config & KLFLatexSyntaxHighlighter::Enabled);
  kccSHKeyword->setColor(_synthighlighterptr->cKeyword);
  kccSHComment->setColor(_synthighlighterptr->cComment);
  chkSHHighlightParensOnly->setChecked(_synthighlighterptr->config & KLFLatexSyntaxHighlighter::HighlightParensOnly);
  kccSHParenMatch->setColor(_synthighlighterptr->cParenMatch);
  kccSHParenMismatch->setColor(_synthighlighterptr->cParenMismatch);

  kfontAppearFont->setFont(_mainwin->txeLatexFont());
  
  btnCancel->setIconSet(QIconSet(locate("appdata", "pics/closehide.png")));
  btnOk->setIconSet(QIconSet(locate("appdata", "pics/ok.png")));

  QSize m = minimumSize();
  m.setWidth(m.width()>500 ? m.width() : 500);
  setMinimumSize(m);
}

KLFSettings::~KLFSettings()
{
}



void KLFSettings::accept()
{
  // apply settings here

  _ptr->tempdir = kurlTempDir->url();
  _ptr->latexexec = kurlLatex->url();
  _ptr->dvipsexec = kurlDvips->url();
  _ptr->gsexec = kurlGs->url();
  _ptr->epstopdfexec = QString();
  if (chkEpstopdf->isChecked()) {
    _ptr->epstopdfexec = kurlEpstopdf->url();
  }
  _ptr->lborderoffset = spnLBorderOffset->value();
  _ptr->tborderoffset = spnTBorderOffset->value();
  _ptr->rborderoffset = spnRBorderOffset->value();
  _ptr->bborderoffset = spnBBorderOffset->value();

  if (chkSHEnabled->isChecked())
    _synthighlighterptr->config |= KLFLatexSyntaxHighlighter::Enabled;
  else
    _synthighlighterptr->config &= ~KLFLatexSyntaxHighlighter::Enabled;
  if (chkSHHighlightParensOnly->isChecked())
    _synthighlighterptr->config |= KLFLatexSyntaxHighlighter::HighlightParensOnly;
  else
    _synthighlighterptr->config &= ~KLFLatexSyntaxHighlighter::HighlightParensOnly;

  _synthighlighterptr->cKeyword = kccSHKeyword->color();
  _synthighlighterptr->cComment = kccSHComment->color();
  _synthighlighterptr->cParenMatch = kccSHParenMatch->color();
  _synthighlighterptr->cParenMismatch = kccSHParenMismatch->color();

  _mainwin->setTxeLatexFont(kfontAppearFont->font());


  _mainwin->saveSettings();

  // and exit dialog
  QDialog::accept();
}



#include "klfsettings.moc"

