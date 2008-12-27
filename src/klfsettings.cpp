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

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFontDialog>

#include <klfcolorchooser.h>
#include <klfpathchooser.h>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klfsettings.h"


KLFSettings::KLFSettings(KLFBackend::klfSettings *p, KLFMainWin* parent)
  : QDialog(parent), KLFSettingsUI()
{
  setupUi(this);

  _ptr = p;
  _mainwin = parent;

  reset();

  connect(btnPathsReset, SIGNAL(clicked()), this, SLOT(setDefaultPaths()));

}

KLFSettings::~KLFSettings()
{
}

void KLFSettings::show()
{
  reset();
  QDialog::show();
}


void KLFSettings::reset()
{

  pathTempDir->setPath(_ptr->tempdir);
  pathLatex->setPath(_ptr->latexexec);
  pathDvips->setPath(_ptr->dvipsexec);
  pathGs->setPath(_ptr->gsexec);
  pathEpstopdf->setPath(_ptr->epstopdfexec);
  chkEpstopdf->setChecked( ! _ptr->epstopdfexec.isEmpty() );
  spnLBorderOffset->setValue( _ptr->lborderoffset );
  spnTBorderOffset->setValue( _ptr->tborderoffset );
  spnRBorderOffset->setValue( _ptr->rborderoffset );
  spnBBorderOffset->setValue( _ptr->bborderoffset );

  gbxSH->setChecked(klfconfig.SyntaxHighlighter.configFlags & KLFLatexSyntaxHighlighter::Enabled);
  colSHKeyword->setColor(klfconfig.SyntaxHighlighter.colorKeyword);
  colSHComment->setColor(klfconfig.SyntaxHighlighter.colorComment);
  chkSHHighlightParensOnly->setChecked(klfconfig.SyntaxHighlighter.configFlags & KLFLatexSyntaxHighlighter::HighlightParensOnly);
  colSHParenMatch->setColor(klfconfig.SyntaxHighlighter.colorParenMatch);
  colSHParenMismatch->setColor(klfconfig.SyntaxHighlighter.colorParenMismatch);
  chkSHHighlightLonelyParen->setChecked(klfconfig.SyntaxHighlighter.configFlags & KLFLatexSyntaxHighlighter::HighlightLonelyParen);
  colSHLonelyParen->setColor(klfconfig.SyntaxHighlighter.colorLonelyParen);

  btnAppearFont->setFont(klfconfig.UI.latexEditFont);
  connect(btnAppearFont, SIGNAL(clicked()), this, SLOT(slotChangeAppearFont()));

  spnPreviewMaxWidth->setValue(klfconfig.UI.previewTooltipMaxSize.width());
  spnPreviewMaxHeight->setValue(klfconfig.UI.previewTooltipMaxSize.height());

}


void KLFSettings::setDefaultPaths()
{
  // ...
}


void KLFSettings::slotChangeAppearFont()
{
  btnAppearFont->setFont(QFontDialog::getFont(0, btnAppearFont->font(), this));
}

void KLFSettings::accept()
{
  // apply settings here

  _ptr->tempdir = pathTempDir->path();
  _ptr->latexexec = pathLatex->path();
  _ptr->dvipsexec = pathDvips->path();
  _ptr->gsexec = pathGs->path();
  _ptr->epstopdfexec = QString();
  if (chkEpstopdf->isChecked()) {
    _ptr->epstopdfexec = pathEpstopdf->path();
  }
  _ptr->lborderoffset = spnLBorderOffset->value();
  _ptr->tborderoffset = spnTBorderOffset->value();
  _ptr->rborderoffset = spnRBorderOffset->value();
  _ptr->bborderoffset = spnBBorderOffset->value();

  if (gbxSH->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::Enabled;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::Enabled;
  if (chkSHHighlightParensOnly->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightParensOnly;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightParensOnly;
  if (chkSHHighlightLonelyParen->isChecked())
    klfconfig.SyntaxHighlighter.configFlags |= KLFLatexSyntaxHighlighter::HighlightLonelyParen;
  else
    klfconfig.SyntaxHighlighter.configFlags &= ~KLFLatexSyntaxHighlighter::HighlightLonelyParen;

  klfconfig.SyntaxHighlighter.colorKeyword = colSHKeyword->color();
  klfconfig.SyntaxHighlighter.colorComment = colSHComment->color();
  klfconfig.SyntaxHighlighter.colorParenMatch = colSHParenMatch->color();
  klfconfig.SyntaxHighlighter.colorParenMismatch = colSHParenMismatch->color();
  klfconfig.SyntaxHighlighter.colorLonelyParen = colSHLonelyParen->color();

  klfconfig.UI.latexEditFont = btnAppearFont->font();
  _mainwin->setTxtLatexFont(btnAppearFont->font());

  klfconfig.UI.previewTooltipMaxSize = QSize(spnPreviewMaxWidth->value(), spnPreviewMaxHeight->value());

  _mainwin->saveSettings();
  klfconfig.writeToConfig();

  // and exit dialog
  QDialog::accept();
}


