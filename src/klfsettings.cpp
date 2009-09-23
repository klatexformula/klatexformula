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

#include <stdlib.h>

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFontDialog>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

#include <klfcolorchooser.h>
#include <klfpathchooser.h>

#include <klfbackend.h>

#include "klfmainwin.h"
#include "klfconfig.h"
#include "klfsettings.h"

#define REG_SH_TEXTFORMATENSEMBLE(x) \
  _textformats.append( TextFormatEnsemble( & klfconfig.SyntaxHighlighter.fmt##x , \
					   colSH##x, colSH##x##Bg , chkSH##x##B , chkSH##x##I ) );

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define PROG_LATEX "latex.exe"
#  define PROG_DVIPS "dvips.exe"
#  define PROG_GS "gswin32c.exe"
#  define PROG_EPSTOPDF "epstopdf.exe"
static QString standard_extra_paths =
  "C:\\Program Files\\MiKTeX*\\miktex\\bin;C:\\Program Files\\gs\\gs*\\bin";
#else
#  define PROG_LATEX "latex"
#  define PROG_DVIPS "dvips"
#  define PROG_GS "gs"
#  define PROG_EPSTOPDF "epstopdf"
static QString standard_extra_paths = "";
#endif


KLFSettings::KLFSettings(KLFMainWin* parent)
  : QDialog(parent), KLFSettingsUI()
{
  setupUi(this);

  _mainwin = parent;

  reset();

  btns->clear();

  QPushButton *b;
  b = new QPushButton(QIcon(":/pics/closehide.png"), tr("Cancel"), btns);
  btns->addButton(b, QDialogButtonBox::RejectRole);
  connect(b, SIGNAL(clicked()), this, SLOT(reject()));
  b = new QPushButton(QIcon(":/pics/apply.png"), tr("Apply"), btns);
  btns->addButton(b, QDialogButtonBox::ApplyRole);
  connect(b, SIGNAL(clicked()), this, SLOT(apply()));
  b = new QPushButton(QIcon(":/pics/ok.png"), tr("OK"), btns);
  btns->addButton(b, QDialogButtonBox::AcceptRole);
  connect(b, SIGNAL(clicked()), this, SLOT(accept()));

  connect(btnPathsReset, SIGNAL(clicked()), this, SLOT(setDefaultPaths()));

  connect(btnAppFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
  connect(btnAppearFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
  connect(btnAppearPreambleFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));

  REG_SH_TEXTFORMATENSEMBLE(Keyword);
  REG_SH_TEXTFORMATENSEMBLE(Comment);
  REG_SH_TEXTFORMATENSEMBLE(ParenMatch);
  REG_SH_TEXTFORMATENSEMBLE(ParenMismatch);
  REG_SH_TEXTFORMATENSEMBLE(LonelyParen);
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
  KLFBackend::klfSettings s = _mainwin->currentSettings();

  pathTempDir->setPath(QDir::toNativeSeparators(s.tempdir));
  pathLatex->setPath(s.latexexec);
  pathDvips->setPath(s.dvipsexec);
  pathGs->setPath(s.gsexec);
  pathEpstopdf->setPath(s.epstopdfexec);
  chkEpstopdf->setChecked( ! s.epstopdfexec.isEmpty() );
  spnLBorderOffset->setValue( s.lborderoffset );
  spnTBorderOffset->setValue( s.tborderoffset );
  spnRBorderOffset->setValue( s.rborderoffset );
  spnBBorderOffset->setValue( s.bborderoffset );

  chkSHEnable->setChecked(klfconfig.SyntaxHighlighter.configFlags
			  &  KLFLatexSyntaxHighlighter::Enabled);
  chkSHHighlightParensOnly->setChecked(klfconfig.SyntaxHighlighter.configFlags
				       &  KLFLatexSyntaxHighlighter::HighlightParensOnly);
  chkSHHighlightLonelyParen->setChecked(klfconfig.SyntaxHighlighter.configFlags
					&  KLFLatexSyntaxHighlighter::HighlightLonelyParen);

  int k;
  for (k = 0; k < _textformats.size(); ++k) {
    if (_textformats[k].fmt->hasProperty(QTextFormat::ForegroundBrush))
      _textformats[k].fg->setColor(_textformats[k].fmt->foreground().color());
    else
      _textformats[k].fg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::BackgroundBrush))
      _textformats[k].bg->setColor(_textformats[k].fmt->background().color());
    else
      _textformats[k].bg->setColor(QColor());
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontWeight))
      _textformats[k].chkB->setChecked(_textformats[k].fmt->fontWeight() > 60);
    else
      _textformats[k].chkB->setCheckState(Qt::PartiallyChecked);
    if (_textformats[k].fmt->hasProperty(QTextFormat::FontItalic))
      _textformats[k].chkI->setChecked(_textformats[k].fmt->fontItalic());
    else
      _textformats[k].chkI->setCheckState(Qt::PartiallyChecked);
  }

  btnAppFont->setFont(klfconfig.UI.applicationFont);
  btnAppearFont->setFont(klfconfig.UI.latexEditFont);
  btnAppearPreambleFont->setFont(klfconfig.UI.preambleEditFont);

  spnPreviewWidth->setValue(klfconfig.UI.labelOutputFixedSize.width());
  spnPreviewHeight->setValue(klfconfig.UI.labelOutputFixedSize.height());

  spnTooltipMaxWidth->setValue(klfconfig.UI.previewTooltipMaxSize.width());
  spnTooltipMaxHeight->setValue(klfconfig.UI.previewTooltipMaxSize.height());

}



bool KLFSettings::setDefaultFor(const QString& progname, const QString& guessedprog, bool required,
				KLFPathChooser *destination)
{
  QString progpath = guessedprog;
  if (progpath.isNull()) {
    if (QFileInfo(destination->path()).isExecutable()) {
      // field already has a valid value, don't touch it and don't complain
      return true;
    }
    if ( ! required )
      return false;
    QMessageBox msgbox(QMessageBox::Critical, tr("Error"), tr("Could not find %1 executable !").arg(progname),
		       QMessageBox::Ok);
    msgbox.setInformativeText(tr("Please check your installation and specify the path"
				 " to %1 executable manually if it is not installed"
				 " in $PATH.").arg(progname));
    msgbox.setDefaultButton(QMessageBox::Ok);
    msgbox.setEscapeButton(QMessageBox::Ok);
    msgbox.exec();
    return false;
  }

  destination->setPath(progpath);
  return true;
}

void KLFSettings::setDefaultPaths()
{
  KLFConfig tempobject;
  KLFConfig::loadDefaultBackendPaths(&tempobject);
  if ( ! QFileInfo(pathTempDir->path()).isDir() )
    pathTempDir->setPath(QDir::toNativeSeparators(tempobject.BackendSettings.tempDir));
  setDefaultFor("latex", tempobject.BackendSettings.execLatex, true, pathLatex);
  setDefaultFor("dvips", tempobject.BackendSettings.execDvips, true, pathDvips);
  setDefaultFor("gs", tempobject.BackendSettings.execGs, true, pathGs);
  bool r = setDefaultFor("epstopdf", tempobject.BackendSettings.execEpstopdf, false, pathEpstopdf);
  chkEpstopdf->setChecked(r);
}


void KLFSettings::slotChangeFont()
{
  QWidget *w = dynamic_cast<QWidget*>(sender());
  if ( w == 0 )
    return;
  w->setFont(QFontDialog::getFont(0, w->font(), this));
}

void KLFSettings::apply()
{
  // apply settings here

  // create a temporary settings object
  KLFBackend::klfSettings s;

  s.tempdir = QDir::fromNativeSeparators(pathTempDir->path());
  s.latexexec = pathLatex->path();
  s.dvipsexec = pathDvips->path();
  s.gsexec = pathGs->path();
  s.epstopdfexec = QString();
  if (chkEpstopdf->isChecked()) {
    s.epstopdfexec = pathEpstopdf->path();
  }
  s.lborderoffset = spnLBorderOffset->value();
  s.tborderoffset = spnTBorderOffset->value();
  s.rborderoffset = spnRBorderOffset->value();
  s.bborderoffset = spnBBorderOffset->value();

  _mainwin->applySettings(s);

  if (chkSHEnable->isChecked())
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

  int k;
  for (k = 0; k < _textformats.size(); ++k) {
    QColor c = _textformats[k].fg->color();
    if (c.isValid())
      _textformats[k].fmt->setForeground(c);
    else
      _textformats[k].fmt->clearForeground();
    c = _textformats[k].bg->color();
    if (c.isValid())
      _textformats[k].fmt->setBackground(c);
    else
      _textformats[k].fmt->clearBackground();
    Qt::CheckState b = _textformats[k].chkB->checkState();
    if (b == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontWeight);
    else if (b == Qt::Checked)
      _textformats[k].fmt->setFontWeight(QFont::Bold);
    else
      _textformats[k].fmt->setFontWeight(QFont::Normal);
    Qt::CheckState it = _textformats[k].chkI->checkState();
    if (it == Qt::PartiallyChecked)
      _textformats[k].fmt->clearProperty(QTextFormat::FontItalic);
    else
      _textformats[k].fmt->setFontItalic( it == Qt::Checked );
  }

  klfconfig.UI.applicationFont = btnAppFont->font();
  qApp->setFont(klfconfig.UI.applicationFont);
  klfconfig.UI.latexEditFont = btnAppearFont->font();
  _mainwin->setTxtLatexFont(klfconfig.UI.latexEditFont);
  klfconfig.UI.preambleEditFont = btnAppearPreambleFont->font();
  _mainwin->setTxtPreambleFont(klfconfig.UI.preambleEditFont);
  // recalculate window sizes etc.
  _mainwin->refreshWindowSizes();

  klfconfig.UI.labelOutputFixedSize = QSize(spnPreviewWidth->value(), spnPreviewHeight->value());

  klfconfig.UI.previewTooltipMaxSize = QSize(spnTooltipMaxWidth->value(), spnTooltipMaxHeight->value());

  _mainwin->saveSettings();
}

void KLFSettings::accept()
{
  // apply settings
  apply();
  // and exit dialog
  QDialog::accept();
}


