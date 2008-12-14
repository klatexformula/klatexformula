/***************************************************************************
 *   file klfmainwin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#include <stack>

#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QImage>
#include <QPushButton>
#include <QFile>
#include <QFont>
#include <QClipboard>
#include <QDrag>
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

#include <klfbackend.h>

#include "klfdata.h"
#include "klfmainwinui.h"
#include "klflibrary.h"
#include "klflatexsymbols.h"
#include "klfsettings.h"
#include "klfmainwin.h"
#include "klfstylemanager.h"



#define min_i(x,y) ( (x)<(y) ? (x) : (y) )




// KLatexFormula version
extern const char version[];
extern const int version_maj, version_min;




KLFLatexSyntaxHighlighter::KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent)
  : QSyntaxHighlighter(parent) , _textedit(textedit)
{
  setDocument(textedit->document());

  _caretpara = 0;
  _caretpos = 0;
}

KLFLatexSyntaxHighlighter::~KLFLatexSyntaxHighlighter()
{
}

void KLFLatexSyntaxHighlighter::setCaretPos(int para, int pos)
{
  _caretpara = para;
  _caretpos = pos;
}

void KLFLatexSyntaxHighlighter::refreshAll()
{
  int pa, po;
...  _textedit->getCursorPosition(&pa, &po);
...  setCaretPos(pa, po);
  rehighlight();
}

void KLFLatexSyntaxHighlighter::parseEverything()
{
  QString text;
  int para = 0;
  uint i = 0;
  QValueList<uint> paralens; // the length of each paragraph
  std::stack<ParenItem> parens; // the parens that we'll meet

  _rulestoapply.clear();
  uint k;
  while (para < textEdit()->paragraphs()) {
    text = textEdit()->text(para);
    i = 0;
    paralens.append(text.length());

    for (i = 0; i < text.length(); ++i) {
      if (text[i] == '\\') {
	++i;
	k = 0;
	if (i >= text.length()) continue;
	if ( (text[i] >= 'a' && text[i+k] <= 'z') || (text[i+k] >= 'A' && text[i+k] <= 'Z') )
	  while (i+k < text.length() && ( (text[i+k] >= 'a' && text[i+k] <= 'z') || (text[i+k] >= 'A' && text[i+k] <= 'Z') ))
	    ++k;
	else
	  k = 1;
	_rulestoapply.append(ColorRule(para, i-1, k+1, klfconfig.SyntaxHighlighter.colorKeyword));
	i += k-1;
      }
      if (text[i] == '%') {
	k = 0;
	while (i+k < text.length())
	  ++k;
	_rulestoapply.append(ColorRule(para, i, k, klfconfig.SyntaxHighlighter.colorComment));
	i += k;
      }
      if (text[i] == '{' || text[i] == '(' || text[i] == '[') {
	bool l = false;
	if (i >= 5 && text.mid(i-5, 5) == "\\left") {
	  l = true;
	}
	parens.push(ParenItem(para, i, (_caretpos == i && (int)_caretpara == para), text[i].latin1(), l));
      }
      if (text[i] == '}' || text[i] == ')' || text[i] == ']') {
	ParenItem p;
	if (!parens.empty()) {
	  p = parens.top();
	  parens.pop();
	} else {
	  p = ParenItem(0, 0, false, '!'); // simulate an item
	  if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	    _rulestoapply.append(ColorRule(para, i, 1, klfconfig.SyntaxHighlighter.colorLonelyParen));
	}
	QColor col = klfconfig.SyntaxHighlighter.colorParenMatch;
	bool l = ( i >= 6 && text.mid(i-6, 6) == "\\right" );
	if ((text[i] == '}' && p.ch != '{') ||
	    (text[i] == ')' && p.ch != '(') ||
	    (text[i] == ']' && p.ch != '[') ||
	    (l != p.left) ) {
	  col = klfconfig.SyntaxHighlighter.colorParenMismatch;
	}
	// does this rule span multiple paragraphs, and do we need to show it (eg. cursor right after paren)
	if (p.highlight || (_caretpos == i+1 && (int)_caretpara == para)) {
	  if ((klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) == 0) {
	    if (p.para != para) {
	      k = p.para;
	      _rulestoapply.append(ColorRule(p.para, p.pos, paralens[p.para]-p.pos, col));
	      while (++k < (uint)para) {
		_rulestoapply.append(ColorRule(k, 0, paralens[k], col)); // fill paragraph with color
	      }
	      _rulestoapply.append(ColorRule(para, 0, i+1, col));
	    } else {
	      _rulestoapply.append(ColorRule(para, p.pos, i-p.pos+1, col));
	    }
	  } else {
	    if (p.ch != '!') // simulated item for first pos
	      _rulestoapply.append(ColorRule(p.para, p.pos, 1, col));
	    _rulestoapply.append(ColorRule(para, i, 1, col));
	  }
	}
      }
    }

    ++para;
  }

  while ( ! parens.empty() ) {
    // if we have unclosed parens
    ParenItem p = parens.top();
    parens.pop();
    if (_caretpara == (uint)p.para && _caretpos == (uint)p.pos) {
      if ( (klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) != 0 )
	_rulestoapply.append(ColorRule(p.para, p.pos, 1, klfconfig.SyntaxHighlighter.colorParenMismatch));
      else {
	_rulestoapply.append(ColorRule(p.para, p.pos, paralens[p.para]-p.pos, klfconfig.SyntaxHighlighter.colorParenMismatch));
	for (k = p.para+1; (int)k < para; ++k) {
	  _rulestoapply.append(ColorRule(k, 0, paralens[k], klfconfig.SyntaxHighlighter.colorParenMismatch));
	}
      }
    } else { // not on caret positions
      if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen) {
	_rulestoapply.append(ColorRule(p.para, p.pos, 1, klfconfig.SyntaxHighlighter.colorLonelyParen));
      }
    }
  }
}


int KLFLatexSyntaxHighlighter::highlightParagraph(const QString& text, int endstatelastpara)
{
  if ( ( klfconfig.SyntaxHighlighter.configFlags & Enabled ) == 0)
    return 0; // forget everything about synt highlight if we don't want it.

  if (endstatelastpara == -2) {
    // first paragraph: parse everything
    _paracount = 0;
    parseEverything();
  }

  setFormat(0, text.length(), QColor(0,0,0));

  for (uint k = 0; k < _rulestoapply.size(); ++k) {
    if (_rulestoapply[k].para == _paracount) {
      // apply rule
      setFormat(_rulestoapply[k].pos, _rulestoapply[k].len, _rulestoapply[k].color);
    }
  }

  _paracount++;

  return 0;
}


// ------------------------------------------------------------------------

KLFProgErr::KLFProgErr(QWidget *parent, QString errtext) : KLFProgErrUI(parent)
{
  txtError->setText(errtext);
}
KLFProgErr::~KLFProgErr()
{
}

void KLFProgErr::showError(QWidget *parent, QString errtext)
{
  KLFProgErr dlg(parent, errtext);
  dlg.exec();
}

// ----------------------------------------------------------------------------






KLFMainWin::KLFMainWin()
 : KLFMainWinUI()
{
  loadSettings();

  _output.status = 0;
  _output.errorstr = QString();
  _output.result = QImage();
  _output.pngdata = QByteArray();
  _output.epsdata = QByteArray();
  _output.pdfdata = QByteArray();

  // load styless
  loadStyles();
  // load library
  loadLibrary();

  _libraryBrowserIsShown = false;
  _latexSymbolsIsShown = false;

  mLibraryBrowser = new KLFLibraryBrowser(&_library, &_libresources, this); // Don't pass NULL here

  mLatexSymbols = new KLFLatexSymbols(this);

  // version information as tooltip on the welcome widget
  lblPromptMain->setToolTip(tr("KLatexFormula %1").arg(QString("")+version));

  txtLatex->setFont(klfconfig.Appearance.latexEditFont);

  frmOutput->setEnabled(false);

  mHighlighter = new KLFLatexSyntaxHighlighter(txeLatex, this);
  mPreambleHighlighter = new KLFLatexSyntaxHighlighter(txtPreamble, this);

  connect(txtLatex, SIGNAL(cursorPositionChanged()),
	  mHighlighter, SLOT(refreshAll()));
  connect(txtPreamble, SIGNAL(cursorPositionChanged()),
	  mPreambleHighlighter, SLOT(refreshAll()));

  _shrinkedsize = frmMain->sizeHint() + QSize(3, 3);
  _expandedsize.setWidth(frmMain->sizeHint().width() + frmDetails->sizeHint().width() + 3);
  _expandedsize.setHeight(frmMain->sizeHint().height() + 3);

  lblOutput->setFixedSize(klfconfig.Appearance.labelOutputFixedSize);

  //  ..............
  //  KHelpMenu *helpMenu = new KHelpMenu(this, klfaboutdata);
  //  mMainWidget->btnHelp->setPopup(helpMenu->menu());

  frmDetails->hide();

  lblOutput->installEventFilter(this);
  txtLatex->installEventFilter(this);

  setFixedSize(_shrinkedsize);

  // Create our style manager
  mStyleManager = new KLFStyleManager(&_styles, this);
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(refreshStylePopupMenus()));
  connect(this, SIGNAL(stylesChanged()), mStyleManager, SLOT(stylesChanged()));

  connect(this, SIGNAL(stylesChanged()), this, SLOT(saveStyles()));
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(saveStyles()));

  // load style "default" or "Default" if one of them exists
  loadNamedStyle("default");
  loadNamedStyle("Default");
  loadNamedStyle(tr("default"));
  loadNamedStyle(tr("Default"));

  // For systematical syntax highlighting
  // make sure syntax highlighting is up-to-date at all times
  QTimer *synthighlighttimer = new QTimer(this);
  synthighlighttimer->start(250);
  connect(synthighlighttimer, SIGNAL(timeout()), mHighlighter, SLOT(refreshAll()));
  connect(synthighlighttimer, SIGNAL(timeout()), mPreambleHighlighter, SLOT(refreshAll()));


  // -- MAJOR SIGNAL/SLOT CONNECTIONS --

  connect(btnClear, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(btnEvaluate, SIGNAL(clicked()), this, SLOT(slotEvaluate()));
  connect(btnSymbols, SIGNAL(toggled(bool)), this, SLOT(slotSymbols(bool)));
  connect(btnLibrary, SIGNAL(toggled(bool)), this, SLOT(slotLibrary(bool)));
  connect(btnExpand, SIGNAL(clicked()), this, SLOT(slotExpandOrShrink()));
  connect(btnCopy, SIGNAL(clicked()), this, SLOT(slotCopy()));
  connect(btnDrag, SIGNAL(released()), this, SLOT(slotDrag()));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(slotSave()));
  connect(btnSettings, SIGNAL(clicked()), this, SLOT(slotSettings()));
  connect(btnQuit, SIGNAL(clicked()), this, SLOT(close()));
  connect(btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  connect(mLibraryBrowser, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)),
	  this, SLOT(restoreFromLibrary(KLFData::KLFLibraryItem, bool)));
  connect(mLibraryBrowser, SIGNAL(refreshLibraryBrowserShownState(bool)),
	  this, SLOT(slotLibraryButtonRefreshState(bool)));
  connect(mLatexSymbols, SIGNAL(insertSymbol(QString,QString)), this, SLOT(insertSymbol(QString,QString)));
  connect(mLatexSymbols, SIGNAL(refreshSymbolBrowserShownState(bool)),
	  this, SLOT(slotSymbolsButtonRefreshState(bool)));

}


KLFMainWin::~KLFMainWin()
{
  saveSettings();
  saveStyles();
  saveLibrary();

  if (mStyleMenu)
    delete mStyleMenu;
  if (mLatexSymbols)
    delete mLatexSymbols;
  if (mLibraryBrowser)
    delete mLibraryBrowser; // we aren't its parent
  if (mPreambleHighlighter)
    delete mPreambleHighlighter;
  if (mHighlighter)
    delete mHighlighter;
}


bool KLFMainWin::loadNamedStyle(const QString& sty)
{
  // find style with name sty (if existant) and set it
  for (uint kl = 0; kl < _styles.size(); ++kl) {
    if (_styles[kl].name == sty) {
      slotLoadStyle(kl);
      return true;
    }
  }
  return false;
}

void KLFMainWin::loadSettings()
{
  _settings.tempdir = klfconfig.BackendSettings.tempDir;
  _settings.latexexec = klfconfig.BackendSettings.execLatex;
  _settings.dvipsexec = klfconfig.BackendSettings.execDvips;
  _settings.gsexec = klfconfig.BackendSettings.execGs;
  _settings.epstopdfexec = klfconfig.BackendSettings.execEpstopdf;
  QString clsname = _settings.tempdir + "/klatexformula.cls";
  if ( ! QFile::exists(clsname) ) {
    bool r = QFile::copy(":/latex/klatexformula.cls", clsname);
    if ( ! r ) {
      QMessageBox::critical(this, tr("Error"), tr("Can't install klatexformula.cls to temporary directory !"));
      ::exit(255);
    }
  }
  _settings.klfclspath = clsname;
  _settings.klfclspath = _settings.klfclspath.left(_settings.klfclspath.findRev('/'));

  _settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  _settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  _settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  _settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;
}

void KLFMainWin::saveSettings()
{
  klfconfig.writeToConfig();
}

void KLFMainWin::refreshStylePopupMenus()
{
  if (!mStyleMenu)
    mStyleMenu = new QMenu(this);
  mStyleMenu->clear();

  //  mStyleMenu->insertTitle(tr("Available Styles"), 100000);
  QAction a;
  for (uint i = 0; i < _styles.size(); ++i) {
    a = mStyleMenu->addAction(_styles[i].name);
    a->setData(i);
    connect(a, SIGNAL(activated()), this, SLOT(slotLoadStyleAct()));
  }
  mStyleMenu->addSeparator();
  mStyleMenu->addAction(QIcon(":/pix/pics/managestyles.png"), tr("Manage Styles"),
			 this, SLOT(slotStyleManager()), 0 /* accel */);
}

void KLFMainWin::loadStyles()
{
  _styles = KLFData::KLFStyleList(); // empty list to start with
  QString styfname = klfconfig.homeConfigDir + "/styles";
  if ( QFile::exists(styfname) ) {
    QFile fsty(styfname);
    if ( ! fsty.open(QIODevice::ReadOnly) )
      QMessageBox::critical(this, tr("Error"), tr("Error: Unable to load your style list!"));
    else {
      QDataStream str(&fsty);
      QString s1;
      str >> s1;
      // check for KLATEXFORMULA_STYLE_LIST
      if (s1 != "KLATEXFORMULA_STYLE_LIST") {
	QMessageBox::critical(this, tr("Error"), tr("Error: Style file is incorrect or corrupt!\n"));
      } else {
	qint16 vmaj, vmin;
	str >> vmaj >> vmin;

	if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	  QMessageBox::warning(this, tr("Load Styles"),
				     tr("The style file found was created by a more recent version of KLatexFormula.\n"
				        "The process of style loading may fail.")
			      );
	}
	
	if (vmaj <= 2) {
	  str.setVersion(QDataStream::Qt_3_3);
	} else {
	  qint16 version;
	  str >> version;
	  str.setVersion(version);
	}

	str >> _styles;

      } // bad file format
    }
  } // file exists

  if (_styles.isEmpty()) {
    // if stylelist is empty, populate with default style
    KLFData::KLFStyle s1 = { tr("Default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "\\usepackage{amssymb,amsmath}\n", 1200 };
    _styles.append(s1);
  }

  mStyleMenu = 0;
  refreshStylePopupMenus();

  btnLoadStyle->setPopup(mStyleMenu);
}

void KLFMainWin::saveStyles()
{
  KLFConfig::ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + "/styles";
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to styles file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);
  // artificially set a lower version, so as to not handicap anyone who would like to read us with an old version of KLF
  stream.setVersion(QDataStream::Qt_3_3);
  stream << QString("KLATEXFORMULA_STYLE_LIST") << (qint16)version_maj << (qint16)version_min << (qint16)stream.version() << _styles;
}

void KLFMainWin::loadLibrary()
{
  _library = KLFData::KLFLibrary();
  _libresources = KLFData::KLFLibraryResourceList();

  KLFData::KLFLibraryResource hist = { KLFData::LibResource_History, tr("History") };
  _libresources.append(hist);
  KLFData::KLFLibraryResource archive = { KLFData::LibResource_Archive, tr("Archive") };
  _libresources.append(archive);


  QString fname = KLFConfig::homeConfigDir + "/library";
  if ( QFile::exists(fname) ) {
    QFile flib(fname);
    if ( ! flib.open(QIODevice::ReadOnly) ) {
      QMessageBox::critical(this, tr("Error"), tr("Unable to open library file!"));
    } else {
      QDataStream stream(&flib);
      QString s1;
      stream >> s1;
      if (s1 != "KLATEXFORMULA_LIBRARY") {
	QMessageBox::critical(this, tr("Error"), tr("Error: Library file is incorrect or corrupt!\n"));
      } else {
	Q_INT16 vmaj, vmin;
	stream >> vmaj >> vmin;

	if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	  QMessageBox::warning(this, tr("Load Library"),
			       tr("The library file found was created by a more recent version of KLatexFormula.\n"
				  "The process of library loading may fail.")
			      );
	}
	
	if (vmaj <= 2) {
	  stream.setVersion(QDataStream::Qt_3_3);
	} else {
	  qint16 version;
	  str >> version;
	  str.setVersion(version);
	}

	stream >> KLFData::KLFLibraryItem::MaxId >> _libresources >> _library;
  
      } // format corrupt
    } // open file ok
  }
  else {
    /* IMPORT PRE-2.1 HISTORY >>>>>>>> */
    QString fname = locate("appdata", "history");
    if ( ! fname.isEmpty() ) {
      QFile fhist(fname);
      if ( ! fhist.open(QIODevice::ReadOnly) ) {
	KMessageBox::error(this, tr("Unable to load your formula history list!"), tr("Error"));
      } else {
	QDataStream stream(&fhist);
	QString s1;
	stream >> s1;
	if (s1 != "KLATEXFORMULA_HISTORY") {
	  KMessageBox::error(this, tr("Error: History file is incorrect or corrupt!\n"), tr("Error"));
	} else {
	  Q_INT16 vmaj, vmin;
	  stream >> vmaj >> vmin;
  
	  if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	    KMessageBox::information(this, tr("The history file found was created by a more recent version of KLatexFormula.\n"
						"The process of history loading may fail."),  tr("Load history"));
	  }

	  stream.setVersion(QDataStream::Qt_3_3);

	  KLFData::KLFLibraryList history;
  
	  stream >> KLFData::KLFLibraryItem::MaxId >> history;

	  _library[_libresources[0]] = history;
  
	} // format corrupt
      } // open file ok
    } // file exists
    /* <<<<<< END IMPORT PRE-2.1 HISTORY */
  }

  emit libraryAllChanged();
}

void KLFMainWin::saveLibrary()
{
  KLFConfig::ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + "/library";
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to library file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);
  // artificially set a lower version, so as to not handicap anyone who would like to read us with an old version of KLF
  stream.setVersion(QDataStream::Qt_3_3);
  stream << QString("KLATEXFORMULA_LIBRARY") << (qint16)version_maj << (qint16)version_min << (qint16)stream.version()
	 << KLFData::KLFLibraryItem::MaxId << _libresources << _library;
}

void KLFMainWin::restoreFromLibrary(KLFData::KLFLibraryItem j, bool restorestyle)
{
  txtLatex->setText(j.latex);
  if (restorestyle) {
    colFg->setColor(QColor(qRed(j.style.fg_color), qGreen(j.style.fg_color), qBlue(j.style.fg_color)));
    colBg->setColor(QColor(qRed(j.style.bg_color), qGreen(j.style.bg_color), qBlue(j.style.bg_color)));
    chkBgTransparent->setChecked(qAlpha(j.style.bg_color) == 0);
    chkMathMode->setChecked(j.style.mathmode.simplifyWhiteSpace() != "...");
    if (j.style.mathmode.simplifyWhiteSpace() != "...")
      cbxMathMode->setCurrentText(j.style.mathmode);
    txtPreamble->setText(j.style.preamble);
    spnDPI->setValue(j.style.dpi);
  }

  lblOutput->setPixmap(j.preview);
  frmOutput->setEnabled(false);
}
 
void KLFMainWin::slotLibraryButtonRefreshState(bool on)
{
  btnLibrary->setOn(on);
  if (...............kapp->sessionSaving()) {
    // if we're saving the session, remember that history browser is on
  } else {
    _libraryBrowserIsShown = on;
  }
}


void KLFMainWin::insertSymbol(QString s, QString xtrapreamble)
{
  txtLatex->insert(s);
  // see if we need to insert the xtrapreamble
  QStringList cmds = QStringList::split("%%", xtrapreamble);
  uint k;
  QString preambletext = txtPreamble->text();
  for (k = 0; k < cmds.size(); ++k) {
    if (preambletext.find(cmds[k]) == -1) {
      if (preambletext[preambletext.length()-1] != '\n')
	preambletext += "\n";
      preambletext += cmds[k];
    }
  }
  txtPreamble->setText(preambletext);
}

void KLFMainWin::slotSymbolsButtonRefreshState(bool on)
{
  btnSymbols->setOn(on);
  if (...................kapp->sessionSaving()) {
    // if we're saving the session, remember that latex symbols is on
  } else {
    _latexSymbolsIsShown = on;
  }
}

bool KLFMainWin::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == txtLatex) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      if (ke->key() == Key_Return && (ke->state() == ShiftButton || ke->state() == ControlButton)) {
	slotEvaluate();
	return true;
      }
    }
  }
  if (obj == lblOutput) {
    if (e->type() == QEvent::MouseMove) {
      slotDrag();
    }
  }
  return KLFMainWinUI::eventFilter(obj, e);
}


void KLFMainWin::slotEvaluate()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  btnEvaluate->setEnabled(false); // don't allow user to click us while we're not done, and
  //				     additionally this gives visual feedback to the user

  input.latex = txtLatex->text();
  if (chkMathMode->isChecked()) {
    input.mathmode = cbxMathMode->currentText();
    if (cbxMathMode->findText(input.mathmode) == -1) {
      cbxMathMode->addItem(input.mathmode);
    }
  } else {
    input.mathmode = "...";
  }
  input.preamble = txtPreamble->text();
  input.fg_color = colFg->color().rgb();
  if (chkBgTransparent->isChecked() == false)
    input.bg_color = colBg->color().rgb();
  else
    input.bg_color = qRgba(255, 255, 255, 0);

  input.dpi = spnDPI->value();

  // and GO !
  _output = KLFBackend::getLatexFormula(input, _settings);

  if (_output.status < 0) {
    QMessageBox::critical(this, tr("Error"), _output.errorstr);
    lblOutput->setText("");
    lblOutput->setPixmap(QPixmap());
    frmOutput->setEnabled(false);
  }
  if (_output.status > 0) {
    KLFProgErr::showError(this, _output.errorstr);
    frmOutput->setEnabled(false);
  }
  if (_output.status == 0) {
    // ALL OK

    QPixmap sc = QPixmap::fromImage(_output.result.scaled(lblOutput->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation))
    mMainWidget->lblOutput->setPixmap(sc);

    mMainWidget->frmOutput->setEnabled(true);

    KLFData::KLFLibraryItem jj = { KLFData::KLFLibraryItem::MaxId++, QDateTime::currentDateTime(), input.latex, sc,
				   KLFData::categoryFromLatex(input.latex), KLFData::tagsFromLatex(input.latex),
				   KLFData::KLFStyle() };
    jj.style = currentStyle();

    if ( ! (_library[_libresources[0]].last() == jj) )
      mLibraryBrowser->addToHistory(jj);

    QSize goodsize = _output.result.size();
    QImage img = _output.result;
    if ( klfconfig.Appearance.previewTooltipMaxSize != QSize(0, 0) && // QSize(0,0) meaning no resize
	 ( img.width() > klfconfig.Appearance.previewTooltipMaxSize.width() ||
	   img.height() > klfconfig.Appearance.previewTooltipMaxSize.height() ) ) {
      img = _output.result.scaled(klfconfig.Appearance.previewTooltipMaxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QMimeSourceFactory::defaultFactory()->setImage( "klfoutput", img );
    lblOutput->setToolTip(QString("<qt><img src=\"klfoutput\"></qt>"));
  }

  btnEvaluate->setEnabled(true); // re-enable our button

  // and save our history and styles
  saveLibrary();
  saveStyles();

}

void KLFMainWin::slotClear()
{
  txtLatex->setText("");
  txtLatex->setFocus();
}

void KLFMainWin::slotLibrary(bool showlib)
{
  mLibraryBrowser->setShown(showlib);
  slotLibraryButtonRefreshState(showlib);
}

void KLFMainWin::slotSymbols(bool showsymbs)
{
  mLatexSymbols->setShown(showsymbs);
  slotSymbolsButtonRefreshState(showsymbs);
}

void KLFMainWin::slotExpandOrShrink()
{
  if (frmDetails->isVisible()) {
    frmDetails->hide();
    setFixedSize(_shrinkedsize);
    btnExpand->setIcon(QIcon(":/pics/switchexpanded.png"));
  } else {
    setFixedSize(_expandedsize);
    frmDetails->show();
    btnExpand->setIcon(QIcon(":/pics/switchshrinked.png"));
  }
}

void KLFMainWin::slotQuit()
{
  close();
}

void KLFMainWin::slotDrag()
{
  QDrag *drag = new QDrag(this);
  QMimeData *mime = new QMimeData;
  mime->setImageData(_output.result);
  drag->setMimeData(mime);
  QImage img;
  img = _output.result.scaled(QSize(200, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  drag->setPixmap(QPixmap::fromImage(img), QPoint(20, 10));
  drag->exec();
}
void KLFMainWin::slotCopy()
{
  QApplication::clipboard()->setImage(_output.result, QClipboard::Clipboard);
}
void KLFMainWin::slotSave()
{
  QString filterformat = "";
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();

  for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
    if ( ! filterformat.isEmpty() ) filterformat += ";;";
    filterformat += tr("%1 Image (*.%1)").arg((*it).lower()).arg((*it).lower());
  }

  filterformat = tr("EPS PostScript (*.eps)")+";;"+filterformat;
  filterformat = tr("PDF Portable Document Format (*.pdf)")+";;"+filterformat;
  filterformat = tr("Standard JPEG Image (*.jpg *.jpeg)")+";;"+filterformat;
  QString selectedfilter = tr("Standard PNG Image (*.png)");
  filterformat = selectedfilter+";;"+filterformat;

  QString fname, format, selectedfilter;
  QString s = QFileDialog::getSaveFileName(this, tr("Save Image Formula"), KLFConfig.UI.lastSaveDir, filterformat,
					   &selectedfilter);
  KLFConfig.UI.lastSaveDir = ............;
  format = ......................;

  if (fname.isEmpty())
    return;

  // The Qt dialog already asks user to confirm overwriting existing files

  QByteArray *dataptr = 0;
  if (format == "ps" || format == "eps") {
    dataptr = &_output.epsdata;
  } else if (format == "pdf") {
    dataptr = &_output.pdfdata;
  } else if (format == "png") {
    dataptr = 0;
    //dataptr = &_output.pngdata;
  } else {
    dataptr = 0;
  }
  
  if (dataptr) {
    if (dataptr->size() == 0) {
      QMessageBox::critical(this, tr("Error"), tr("Sorry, format `%1' is not available.").arg(format));
      slotSave(); // recurse and prompt user.
      return;
    }
    QFile fsav(fname);
    if ( ! fsav.open(QIODevice::WriteOnly) ) {
      QMessageBox::critical(this, tr("Error"), tr("Error: Can't write to file %1!").arg(fname));
      return;
    }
    fsav.writeBlock(*dataptr);
  } else {
    // add text information for latex formula, style, etc.
    // with QImageWriter
    ..........
    _output.result.save(fname, format.upper());
  }

}


KLFData::KLFStyle KLFMainWin::currentStyle() const
{
  KLFData::KLFStyle sty;

  sty.name = QString::null;
  sty.fg_color = colFg->color().rgb();
  QColor bgc = colBg->color();
  sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), chkBgTransparent->isChecked() ? 0 : 255 );
  sty.mathmode = (chkMathMode->isChecked() ? cbxMathMode->currentText() : "...");
  sty.preamble = txtPreamble->text();
  sty.dpi = spnDPI->value();

  return sty;
}

void KLFMainWin::slotLoadStyleAct()
{
  QAction *a = qobject_cast<QAction*>(sender());
  if (a) {
    slotLoadStyle(a->data().toInt());
  }
}

void KLFMainWin::slotLoadStyle(int n)
{
  if (n < 0 || n >= (int)_styles.size())
    return; // let's not try to load an inexistant style...

  uint fg = _styles[n].fg_color;
  uint bg = _styles[n].bg_color;
  colFg->setColor(QColor(qRed(fg), qGreen(fg), qBlue(fg)));
  colBg->setColor(QColor(qRed(bg), qGreen(bg), qBlue(bg)));
  chkBgTransparent->setChecked(qAlpha(bg) == 0);
  if (_styles[n].mathmode.simplifyWhiteSpace() == "...") {
    chkMathMode->setChecked(false);
  } else {
    chkMathMode->setChecked(true);
    cbxMathMode->setCurrentText(_styles[n].mathmode);
  }
  txtPreamble->setText(_styles[n].preamble);
  spnDPI->setValue(_styles[n].dpi);
}
void KLFMainWin::slotSaveStyle()
{
  KLFData::KLFStyle sty;

  QString name = QInputDialog::getText(this, tr("Enter Style Name"), tr("Enter new style name:"));
  if (name.isEmpty()) {
    return;
  }

  // check to see if style exists already
  int found_i = -1;
  for (uint kl = 0; found_i == -1 && kl < _styles.size(); ++kl) {
    if (_styles[kl].name.stripWhiteSpace() == name.stripWhiteSpace()) {
      found_i = kl;
      // style exists already
      int r = QMessageBox::question(this, tr("Overwrite Style"), tr("Style name already exists. Do you want to overwrite?"),
				    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::No);
      switch (r) {
      case QMessageBox::No:
	slotSaveStyle(); // recurse, ask for new name.
	return; // and stop this function execution of course!
      case QMessageBox::Yes:
	break; // continue normally
      default:
	return;	// forget everything; "Cancel"
      }
    }
  }

  sty = currentStyle();
  sty.name = name;

  if (found_i == -1)
    _styles.append(sty);
  else
    _styles[found_i] = sty;

  emit stylesChanged();

  refreshStylePopupMenus();

  // auto-save our history & style list
  saveLibrary();
  saveStyles();
}

void KLFMainWin::slotStyleManager()
{
  mStyleManager->show();
}


void KLFMainWin::slotSettings()
{
  static KLFSettings *dlg = 0;
  if (dlg == 0) {
    dlg = new KLFSettings(&_settings, this);
  }

  dlg->show();
}





//.......................................

void KLFMainWin::saveProperties(KConfig *cfg)
{
  saveSettings();
  saveStyles();
  saveLibrary();

  cfg->writeEntry("txeLatex", mMainWidget->txeLatex->text());
  cfg->writeEntry("kccFg", mMainWidget->kccFg->color());
  cfg->writeEntry("chkBgTransparent", (bool)mMainWidget->chkBgTransparent->isChecked());
  cfg->writeEntry("kccBg", mMainWidget->kccBg->color());
  cfg->writeEntry("chkMathMode", (bool)mMainWidget->chkMathMode->isChecked());
  cfg->writeEntry("cbxMathMode", mMainWidget->cbxMathMode->currentText());
  cfg->writeEntry("txePreamble", mMainWidget->txePreamble->text());
  cfg->writeEntry("spnDPI", (int)mMainWidget->spnDPI->value());

  cfg->writeEntry("displayExpandedMode", (bool)mMainWidget->frmDetails->isVisible());

  cfg->writeEntry("displayLibraryBrowserVisible", _libraryBrowserIsShown);
  cfg->writeEntry("displayLatexSymbolsVisible", _latexSymbolsIsShown);
  cfg->writeEntry("displayLibraryBrowserGeometry", mLibraryBrowser->geometry());
  cfg->writeEntry("displayLatexSymbolsGeometry", mLatexSymbols->geometry());

  KMainWindow::saveProperties(cfg);
}
void KLFMainWin::readProperties(KConfig *cfg)
{
  loadSettings();
  loadStyles();
  loadLibrary();

  mMainWidget->txeLatex->setText(cfg->readEntry("txeLatex", ""));
  mMainWidget->kccFg->setColor(cfg->readColorEntry("kccFg"));
  mMainWidget->chkBgTransparent->setChecked(cfg->readBoolEntry("chkBgTransparent", true));
  mMainWidget->kccBg->setColor(cfg->readColorEntry("kccBg"));
  mMainWidget->chkMathMode->setChecked(cfg->readBoolEntry("chkMathMode", true));
  mMainWidget->cbxMathMode->setCurrentText(cfg->readEntry("cbxMathMode"));
  mMainWidget->txePreamble->setText(cfg->readEntry("txePreamble", ""));
  mMainWidget->spnDPI->setValue(cfg->readNumEntry("spnDPI", 1200));

  mMainWidget->frmDetails->setShown(cfg->readBoolEntry("displayExpandedMode", false));

  // history browser geometry
  bool hb_vis = false;
  QRect hb_r = mLibraryBrowser->geometry();
  hb_vis = cfg->readBoolEntry("displayLibraryBrowserVisible", hb_vis);
  hb_r = cfg->readRectEntry("displayLibraryBrowserGeometry", &hb_r);
  slotLibrary(hb_vis);
  mLibraryBrowser->setGeometry(hb_r);
  // latex symbols geometry
  bool ls_vis = false;
  QRect ls_r = mLatexSymbols->geometry();
  ls_vis = cfg->readBoolEntry("displayLatexSymbolsVisible", ls_vis);
  ls_r = cfg->readRectEntry("displayLatexSymbolsGeometry", &ls_r);
  slotSymbols(ls_vis);
  mLatexSymbols->setGeometry(ls_r);


  KMainWindow::readProperties(cfg);
}



#include "klfmainwin.moc"
