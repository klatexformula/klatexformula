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

#include <stdio.h>

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
#include <QProcess>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMimeData>
#include <QImageWriter>
#include <QInputDialog>
#include <QCloseEvent>
#include <QTemporaryFile>
#include <QByteArray>
#include <QBuffer>

#include <klfbackend.h>

#include <ui_klfaboutdialogui.h>
#include <ui_klfprogerrui.h>
#include <ui_klfmainwinui.h>

#include "klfdata.h"
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

  _caretpos = 0;
}

KLFLatexSyntaxHighlighter::~KLFLatexSyntaxHighlighter()
{
}

void KLFLatexSyntaxHighlighter::setCaretPos(int position)
{
  _caretpos = position;
}

void KLFLatexSyntaxHighlighter::refreshAll()
{
  rehighlight();
}

void KLFLatexSyntaxHighlighter::parseEverything()
{
  QString text;
  int i = 0;
  int blockpos;
  QList<uint> blocklens; // the length of each block
  std::stack<ParenItem> parens; // the parens that we'll meet

  QTextBlock block = document()->firstBlock();
  
  _rulestoapply.clear();
  int k;
  while (block.isValid()) {
    text = block.text();
    i = 0;
    blockpos = block.position();
    blocklens.append(block.length());

    while (text.length() < block.length()) {
      text += "\n";
    }

    static QRegExp bsleft("^\\\\left(?!\\w)");
    static QRegExp bsright("^\\\\right(?!\\w)");

    i = 0;
    while ( i < text.length() ) {
      if (text[i] == '%') {
	k = 0;
	while (i+k < text.length() && text[i+k] != '\n')
	  ++k;
	_rulestoapply.append(FormatRule(blockpos+i, k, FComment));
	i += k + 1;
	continue;
      }
      if (bsleft.indexIn(text.mid(i)) != -1 ||
	  text[i] == '{' || text[i] == '(' || text[i] == '[') {
	bool l = (text.mid(i, 5) == "\\left");
	if (l)
	  i += 5;
	if (i == text.length()) // ignore a \left with no following character
	  continue;
	if (text.mid(i,2) == "\\{")
	  ++i; // focus on the '{' sign, not the \\ sign
	parens.push(ParenItem(blockpos+i, (_caretpos == blockpos+i), text[i].toAscii(), l));
	if (i > 0 && text[i-1] == '\\') {
	  --i; // allow the next-next if-block for keywords to highlight this "\\{"
	}
      }
      if (bsright.indexIn(text.mid(i)) != -1 || text[i] == '}' || text[i] == ')' || text[i] == ']') {
	ParenItem p;
	if (!parens.empty()) {
	  p = parens.top();
	  parens.pop();
	} else {
	  p = ParenItem(0, false, '!'); // simulate an item
	  if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	    _rulestoapply.append(FormatRule(blockpos+i, 1, FLonelyParen));
	}
	Format col = FParenMatch;
	bool l = ( text.mid(i, 6) == "\\right" );
	if (l)
	  i += 6;
	if (i == text.length()) // ignore a \right with no following character
	  continue;
	if (text.mid(i,2) == "\\}")
	  ++i; // focus on the '}' sign, not the \\ sign
	if ( (l && text[i] == '.' && p.left) || (l && p.ch == '.' && p.left) ) {
	  // special case with \left( blablabla \right.  or  \left. blablabla \right)
	  col = FParenMatch;
	} else if ((text[i] == '}' && p.ch != '{') ||
	    (text[i] == ')' && p.ch != '(') ||
	    (text[i] == ']' && p.ch != '[') ||
	    (l != p.left) ) {
	  col = FParenMismatch;
	}
	// does this rule span multiple paragraphs, and do we need to show it (eg. cursor right after paren)
	if (p.highlight || (_caretpos == blockpos+i+1)) {
	  if ((klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) == 0) {
	    _rulestoapply.append(FormatRule(p.pos, blockpos+i+1-p.pos, col, true));
	  } else {
	    if (p.ch != '!') // simulated item for first pos
	      _rulestoapply.append(FormatRule(p.pos, 1, col));
	    _rulestoapply.append(FormatRule(blockpos+i, 1, col, true));
	  }
	}
	if (i > 0 && text[i-1] == '\\') {
	  --i; // allow the next if-block for keywords to highlight this "\\}"
	}
      }

      if (text[i] == '\\') {
	++i;
	k = 0;
	if (i >= text.length())
	  continue;
	while (i+k < text.length() && ( (text[i+k] >= 'a' && text[i+k] <= 'z') || (text[i+k] >= 'A' && text[i+k] <= 'Z') ))
	  ++k;
	if (k == 0 && i+1 < text.length())
	  k = 1;
	_rulestoapply.append(FormatRule(blockpos+i-1, k+1, FKeyWord));
	i += k;
	continue;
      }

      ++i;
    }

    block = block.next();
  }

  QTextBlock lastblock = document()->lastBlock();

  while ( ! parens.empty() ) {
    // for each unclosed paren left
    ParenItem p = parens.top();
    parens.pop();
    if (_caretpos == p.pos) {
      if ( (klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) != 0 )
	_rulestoapply.append(FormatRule(p.pos, 1, FParenMismatch, true));
      else
	_rulestoapply.append(FormatRule(p.pos, lastblock.position()+lastblock.length()-p.pos, FParenMismatch, true));
    } else { // not on caret positions
      if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	_rulestoapply.append(FormatRule(p.pos, 1, FLonelyParen));
    }
  }
}

QTextCharFormat KLFLatexSyntaxHighlighter::charfmtForFormat(Format f)
{
  QTextCharFormat fmt;
  switch (f) {
  case FNormal:
    fmt = QTextCharFormat();
    break;
  case FKeyWord:
    fmt = klfconfig.SyntaxHighlighter.fmtKeyword;
    break;
  case FComment:
    fmt = klfconfig.SyntaxHighlighter.fmtComment;
    break;
  case FParenMatch:
    fmt = klfconfig.SyntaxHighlighter.fmtParenMatch;
    break;
  case FParenMismatch:
    fmt = klfconfig.SyntaxHighlighter.fmtParenMismatch;
    break;
  case FLonelyParen:
    fmt = klfconfig.SyntaxHighlighter.fmtLonelyParen;
    break;
  default:
    fmt = QTextCharFormat();
    break;
  };
  return fmt;
}


void KLFLatexSyntaxHighlighter::highlightBlock(const QString& text)
{
  //  printf("-- HIGHLIGHTBLOCK --:\n%s\n", (const char*)text.toLocal8Bit());

  if ( ( klfconfig.SyntaxHighlighter.configFlags & Enabled ) == 0)
    return; // forget everything about synt highlight if we don't want it.

  QTextBlock block = currentBlock();

  //  printf("\t -- block/position=%d\n", block.position());

  if (block.position() == 0) {
    setCaretPos(_textedit->textCursor().position());
    parseEverything();
  }

  QList<FormatRule> blockfmtrules;
  QVector<QTextCharFormat> charformats;

  charformats.resize(text.length());

  blockfmtrules.append(FormatRule(0, text.length(), FNormal));

  int k, j;
  for (k = 0; k < _rulestoapply.size(); ++k) {
    int start = _rulestoapply[k].pos - block.position();
    int len = _rulestoapply[k].len;
    
    if (start < 0) {
      len += start; // +, start being negative
      start = 0;
    }
    if (start > text.length())
      continue;
    if (len > text.length() - start)
      len = text.length() - start;
    // apply rule
    blockfmtrules.append(FormatRule(start, len, _rulestoapply[k].format, _rulestoapply[k].onlyIfFocus));
  }

  bool hasfocus = _textedit->hasFocus();
  for (k = 0; k < blockfmtrules.size(); ++k) {
    for (j = blockfmtrules[k].pos; j < blockfmtrules[k].end(); ++j) {
      if ( ! blockfmtrules[k].onlyIfFocus || hasfocus )
	charformats[j].merge(charfmtForFormat(blockfmtrules[k].format));
    }
  }
  for (j = 0; j < charformats.size(); ++j) {
    setFormat(j, 1, charformats[j]);
  }
  
  return;
}


// ------------------------------------------------------------------------

KLFProgErr::KLFProgErr(QWidget *parent, QString errtext) : QDialog(parent)
{
  mUI = new Ui::KLFProgErrUI;
  mUI->setupUi(this);

  mUI->txtError->setText(errtext);
}

KLFProgErr::~KLFProgErr()
{
  delete mUI;
}

void KLFProgErr::showError(QWidget *parent, QString errtext)
{
  KLFProgErr dlg(parent, errtext);
  dlg.exec();
}



// ----------------------------------------------------------------------------

KLFPreviewBuilderThread::KLFPreviewBuilderThread(QObject *parent, KLFBackend::klfInput input,
						 KLFBackend::klfSettings settings, int labelwidth, int labelheight)
  : QThread(parent), _input(input), _settings(settings), _lwidth(labelwidth), _lheight(labelheight),
    _hasnewinfo(false), _abort(false)
{
}
KLFPreviewBuilderThread::~KLFPreviewBuilderThread()
{
  _mutex.lock();
  _abort = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
  wait();
}

void KLFPreviewBuilderThread::run()
{
  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  KLFBackend::klfOutput output;
  QImage img;
  int lwid, lhgt;

  for (;;) {
    if (_abort)
      return;
    // fetch info
    _mutex.lock();
    input = _input;
    settings = _settings;
    settings.epstopdfexec = "";
    lwid = _lwidth;
    lhgt = _lheight;
    _hasnewinfo = false;
    _mutex.unlock();
    // render equation
    //  no performance improvement noticed with lower DPI:
    //    // force 240 DPI (we're only a preview...)
    //    input.dpi = 240;

    if ( ! input.latex.isEmpty() ) {
      // and GO!
      output = KLFBackend::getLatexFormula(input, settings);
      img = output.result;
      if (output.status == 0) {
	if (img.width() > lwid || img.height() > lhgt)
	  img = img.scaled(QSize(lwid, lhgt), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      } else {
	img = QImage();
      }

      emit previewAvailable(img, output.status != 0);
    }
    if (_abort)
      return;
    if (_hasnewinfo)
      continue;
    _mutex.lock();
    _condnewinfoavail.wait(&_mutex);
    _mutex.unlock();
  }
}
bool KLFPreviewBuilderThread::inputChanged(const KLFBackend::klfInput& input)
{
  QMutexLocker mutexlocker(&_mutex);
  if (_input == input) {
    return false;
  }
  _input = input;
  _hasnewinfo = true;
  _condnewinfoavail.wakeOne();
  return true;
}
void KLFPreviewBuilderThread::settingsChanged(const KLFBackend::klfSettings& settings,
					      int lwidth, int lheight)
{
  _mutex.lock();
  _settings = settings;
  if (lwidth > 0) _lwidth = lwidth;
  if (lheight > 0) _lheight = lheight;
  _hasnewinfo = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
}




// ----------------------------------------------------------------------------



KLFMainWin::KLFMainWin()
  : QWidget(0, Qt::Window), KLFMainWinUI()
{
  setupUi(this);

  loadSettings();

  _output.status = 0;
  _output.errorstr = QString();
  _output.result = QImage();
  _output.pngdata = QByteArray();
  _output.epsdata = QByteArray();
  _output.pdfdata = QByteArray();

  mLastRunTempPNGFile = 0;

  // load styless
  loadStyles();
  // load library
  loadLibrary();

  //  _libraryBrowserIsShown = false;
  //  _latexSymbolsIsShown = false;

  mLibraryBrowser = new KLFLibraryBrowser(&_library, &_libresources, this); // Don't pass NULL here

  mLatexSymbols = new KLFLatexSymbols(this);

  // version information as tooltip on the welcome widget
  lblPromptMain->setToolTip(tr("KLatexFormula %1").arg(QString::fromUtf8(version)));

  txtLatex->setFont(klfconfig.UI.latexEditFont);
  txtPreamble->setFont(klfconfig.UI.preambleEditFont);

  frmOutput->setEnabled(false);

  mHighlighter = new KLFLatexSyntaxHighlighter(txtLatex, this);
  mPreambleHighlighter = new KLFLatexSyntaxHighlighter(txtPreamble, this);

  connect(txtLatex, SIGNAL(cursorPositionChanged()),
	  mHighlighter, SLOT(refreshAll()));
  connect(txtPreamble, SIGNAL(cursorPositionChanged()),
	  mPreambleHighlighter, SLOT(refreshAll()));

  lblOutput->setFixedSize(klfconfig.UI.labelOutputFixedSize);

  refreshWindowSizes();

  frmDetails->hide();

  lblOutput->installEventFilter(this);
  txtLatex->installEventFilter(this);

  setFixedSize(_shrinkedsize);
  // adjustSize();

  // Create our style manager
  mStyleManager = new KLFStyleManager(&_styles, this);
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(refreshStylePopupMenus()));
  connect(this, SIGNAL(stylesChanged()), mStyleManager, SLOT(slotRefresh()));

  connect(this, SIGNAL(stylesChanged()), this, SLOT(saveStyles()));
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(saveStyles()));

  // load style "default" or "Default" (or translation) if one of them exists
  loadNamedStyle("default");
  loadNamedStyle("Default");
  loadNamedStyle(tr("default"));
  loadNamedStyle(tr("Default"));

  // For systematical syntax highlighting
  // make sure syntax highlighting is up-to-date at all times
  QTimer *synthighlighttimer = new QTimer(this);
  connect(synthighlighttimer, SIGNAL(timeout()), mHighlighter, SLOT(refreshAll()));
  connect(synthighlighttimer, SIGNAL(timeout()), mPreambleHighlighter, SLOT(refreshAll()));
  synthighlighttimer->start(250);

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
  connect(btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  connect(mLibraryBrowser, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)),
	  this, SLOT(restoreFromLibrary(KLFData::KLFLibraryItem, bool)));
  connect(mLibraryBrowser, SIGNAL(refreshLibraryBrowserShownState(bool)),
	  this, SLOT(slotLibraryButtonRefreshState(bool)));
  connect(mLatexSymbols, SIGNAL(insertSymbol(const KLFLatexSymbol&)),
	  this, SLOT(insertSymbol(const KLFLatexSymbol&)));
  connect(mLatexSymbols, SIGNAL(refreshSymbolBrowserShownState(bool)),
	  this, SLOT(slotSymbolsButtonRefreshState(bool)));

  // our help dialog

  QDialog *aboutDialog = new QDialog(this);
  Ui::KLFAboutDialogUI *aboutDialogUI = new Ui::KLFAboutDialogUI;
  aboutDialogUI->setupUi(aboutDialog);

  connect(btnHelp, SIGNAL(clicked()), aboutDialog, SLOT(show()));

  // -- SMALL REAL-TIME PREVIEW GENERATOR THREAD --

  mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
						      klfconfig.UI.labelOutputFixedSize.width(),
						      klfconfig.UI.labelOutputFixedSize.height());

  connect(txtLatex, SIGNAL(textChanged()), this, SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(cbxMathMode, SIGNAL(editTextChanged(const QString&)), this, SLOT(updatePreviewBuilderThreadInput()),
	  Qt::QueuedConnection);
  connect(chkMathMode, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(colFg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(chkBgTransparent, SIGNAL(stateChanged(int)), this, SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);
  connect(colBg, SIGNAL(colorChanged(const QColor&)), this, SLOT(updatePreviewBuilderThreadInput()), Qt::QueuedConnection);

  qRegisterMetaType<QImage>("QImage");
  connect(mPreviewBuilderThread, SIGNAL(previewAvailable(const QImage&, bool)),
	  this, SLOT(showRealTimePreview(const QImage&, bool)), Qt::QueuedConnection);

  if (klfconfig.UI.enableRealTimePreview) {
    mPreviewBuilderThread->start();
  }

  // CREATE SETTINGS DIALOG

  mSettingsDialog = new KLFSettings(this);


  // INTERNAL FLAGS

  _evaloutput_uptodate = false;

  _ignore_hide_event = false;
}


KLFMainWin::~KLFMainWin()
{
  //  printf("DEBUG: KLFMainWin::~KLFMainWin(): destroying...\n");

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
  if (mPreviewBuilderThread)
    delete mPreviewBuilderThread;
}


void KLFMainWin::refreshWindowSizes()
{
  bool curstate_shown = frmDetails->isVisible();
  frmDetails->show();
  _shrinkedsize = frmMain->sizeHint() + QSize(3, 3);
  _expandedsize.setWidth(frmMain->sizeHint().width() + frmDetails->sizeHint().width() + 3);
  _expandedsize.setHeight(frmMain->sizeHint().height() + 3);
  if (curstate_shown) {
    setFixedSize(_expandedsize);
    frmDetails->show();
  } else {
    setFixedSize(_shrinkedsize);
    frmDetails->hide();
  }
  updateGeometry();
}


bool KLFMainWin::loadNamedStyle(const QString& sty)
{
  // find style with name sty (if existant) and set it
  for (int kl = 0; kl < _styles.size(); ++kl) {
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

  _settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  _settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  _settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  _settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;

  _settings_altered = false;
}

void KLFMainWin::saveSettings()
{
  if ( ! _settings_altered ) {
    // don't save altered settings
    klfconfig.BackendSettings.tempDir = _settings.tempdir;
    klfconfig.BackendSettings.execLatex = _settings.latexexec;
    klfconfig.BackendSettings.execDvips = _settings.dvipsexec;
    klfconfig.BackendSettings.execGs = _settings.gsexec;
    klfconfig.BackendSettings.execEpstopdf = _settings.epstopdfexec;
    klfconfig.BackendSettings.lborderoffset = _settings.lborderoffset;
    klfconfig.BackendSettings.tborderoffset = _settings.tborderoffset;
    klfconfig.BackendSettings.rborderoffset = _settings.rborderoffset;
    klfconfig.BackendSettings.bborderoffset = _settings.bborderoffset;
  }

  klfconfig.UI.userColorList = KLFColorChooser::colorList();
  klfconfig.UI.colorChooseWidgetRecent = KLFColorChooseWidget::recentColors();
  klfconfig.UI.colorChooseWidgetCustom = KLFColorChooseWidget::customColors();

  klfconfig.writeToConfig();

  mPreviewBuilderThread->settingsChanged(_settings, klfconfig.UI.labelOutputFixedSize.width(),
					 klfconfig.UI.labelOutputFixedSize.height());

  if (klfconfig.UI.enableRealTimePreview) {
    if ( ! mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
							  klfconfig.UI.labelOutputFixedSize.width(),
							  klfconfig.UI.labelOutputFixedSize.height());
      mPreviewBuilderThread->start();
    }
  } else {
    if ( mPreviewBuilderThread->isRunning() ) {
      delete mPreviewBuilderThread;
      // do NOT leave a NULL mPreviewBuilderThread !
      mPreviewBuilderThread = new KLFPreviewBuilderThread(this, collectInput(), _settings,
							  klfconfig.UI.labelOutputFixedSize.width(),
							  klfconfig.UI.labelOutputFixedSize.height());
    }
  }
}

void KLFMainWin::refreshStylePopupMenus()
{
  if (!mStyleMenu)
    mStyleMenu = new QMenu(this);
  mStyleMenu->clear();

  //  mStyleMenu->insertTitle(tr("Available Styles"), 100000);
  QAction *a;
  for (int i = 0; i < _styles.size(); ++i) {
    a = mStyleMenu->addAction(_styles[i].name);
    a->setData(i);
    connect(a, SIGNAL(activated()), this, SLOT(slotLoadStyleAct()));
  }
  mStyleMenu->addSeparator();
  mStyleMenu->addAction(QIcon(":/pix/pics/managestyles.png"), tr("Manage Styles"),
			 this, SLOT(slotStyleManager()), 0 /* accel */);
}

QString kdelocate(const char *fname)
{
  QString candidate;

  QStringList env = QProcess::systemEnvironment();
  QStringList kdehome = env.filter(QRegExp("^KDEHOME="));
  if (kdehome.size() == 0) {
    candidate = QString("~/.kde/share/apps/klatexformula/") + QString::fromLocal8Bit(fname);
  } else {
    QString kdehomeval = kdehome[0];
    kdehomeval.replace(QRegExp("^KDEHOME="), "");
    candidate = kdehomeval + "/share/apps/klatexformula/" + QString::fromLocal8Bit(fname);
  }
  if (QFile::exists(candidate)) {
    return candidate;
  }
  return QString::null;
}

void KLFMainWin::loadStyles()
{
  _styles = KLFData::KLFStyleList(); // empty list to start with
  QString styfname = klfconfig.homeConfigDir + "/styles";
  if ( ! QFile::exists(styfname) ) {
    // try KDE version (for KLF 2.x)
    styfname = kdelocate("styles");
  }
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
    KLFData::KLFStyle s1 = { tr("Default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]",
			     "", 1200 };
    _styles.append(s1);
  }

  mStyleMenu = 0;
  refreshStylePopupMenus();

  btnLoadStyle->setMenu(mStyleMenu);
}

void KLFMainWin::saveStyles()
{
  klfconfig.ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + "/styles";
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to styles file!\n%1").arg(s));
    return;
  }
  QDataStream stream(&f);
  stream.setVersion(QDataStream::Qt_3_3);
  stream << QString("KLATEXFORMULA_STYLE_LIST") << (qint16)version_maj << (qint16)version_min
	 << (qint16)stream.version() << _styles;
}

void KLFMainWin::loadLibrary()
{
  _library = KLFData::KLFLibrary();
  _libresources = KLFData::KLFLibraryResourceList();

  KLFData::KLFLibraryResource hist = { KLFData::LibResource_History, tr("History") };
  _libresources.append(hist);
  KLFData::KLFLibraryResource archive = { KLFData::LibResource_Archive, tr("Archive") };
  _libresources.append(archive);

  // Locate a good library file.

  // the default library file
  QString fname = klfconfig.homeConfigDir + "/library";
  // if unexistant, try to load
  if ( ! QFile::exists(fname) ) {
    // try KDE KLF version
    fname = kdelocate("library");
  }
  if ( ! QFile::exists(fname) ) {
    // next resort: try post-2.0, pre-2.1 "history"
    fname = kdelocate("history");
    // in which case we need a different treatment...
    if ( ! fname.isEmpty() ) {
      /* IMPORT PRE-2.1 HISTORY >>>>>>>> */
      QFile fhist(fname);
      if ( ! fhist.open(QIODevice::ReadOnly) ) {
	QMessageBox::critical(this, tr("Error"), tr("Unable to load your formula history list!"));
      } else {
	QDataStream stream(&fhist);
	QString s1;
	stream >> s1;
	if (s1 != "KLATEXFORMULA_HISTORY") {
	  QMessageBox::critical(this, tr("Error"), tr("Error: History file is incorrect or corrupt!\n"));
	} else {
	  qint16 vmaj, vmin;
	  stream >> vmaj >> vmin;
	  
	  if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	    QMessageBox::information(this, tr("Load History"),
				     tr("The history file found was created by a more recent version of KLatexFormula.\n"
					"The process of history loading may fail."));
	  }
	  
	  stream.setVersion(QDataStream::Qt_3_3);
	  
	  KLFData::KLFLibraryList history;
	  
	  stream >> KLFData::KLFLibraryItem::MaxId >> history;
	  
	  _library[_libresources[0]] = history;
	  
	} // format corrupt
      } // open file ok
      /* <<<<<< END IMPORT PRE-2.1 HISTORY */
      emit libraryAllChanged();
      return;
    }
  }
  if ( ! QFile::exists(fname) ) {
    // as last resort we load our default library stored in a resource
    fname = ":/data/defaultlibrary";
  }

  // LOAD LIBRARY
  
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
      qint16 vmaj, vmin;
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
	stream >> version;
	stream.setVersion(version);
      }
      
      stream >> KLFData::KLFLibraryItem::MaxId >> _libresources >> _library;
      
    } // format corrupt
  } // open file ok

  emit libraryAllChanged();
}

void KLFMainWin::saveLibrary()
{
  // librarysave
  klfconfig.ensureHomeConfigDir();
  QString s = klfconfig.homeConfigDir + "/library";
  QFile f(s);
  if ( ! f.open(QIODevice::WriteOnly) ) {
    QMessageBox::critical(this, tr("Error"), tr("Error: Unable to write to library file `%1'!")
			   .arg(QDir::toNativeSeparators(s)));
    return;
  }
  QDataStream stream(&f);
  stream.setVersion(QDataStream::Qt_3_3);
  // write KLF-3.0-compatible stream
  stream << QString("KLATEXFORMULA_LIBRARY") << (qint16)3 << (qint16)0 << (qint16)stream.version()
	 << KLFData::KLFLibraryItem::MaxId << _libresources << _library;

}

void KLFMainWin::restoreFromLibrary(KLFData::KLFLibraryItem j, bool restorestyle)
{
  txtLatex->setPlainText(j.latex);
  if (restorestyle) {
    colFg->setColor(QColor(qRed(j.style.fg_color), qGreen(j.style.fg_color), qBlue(j.style.fg_color)));
    colBg->setColor(QColor(qRed(j.style.bg_color), qGreen(j.style.bg_color), qBlue(j.style.bg_color)));
    chkBgTransparent->setChecked(qAlpha(j.style.bg_color) == 0);
    chkMathMode->setChecked(j.style.mathmode.simplified() != "...");
    if (j.style.mathmode.simplified() != "...")
      cbxMathMode->setEditText(j.style.mathmode);
    txtPreamble->setPlainText(j.style.preamble);
    spnDPI->setValue(j.style.dpi);
  }

  lblOutput->setPixmap(j.preview);
  frmOutput->setEnabled(false);
  activateWindow();
  raise();
}
 
void KLFMainWin::slotLibraryButtonRefreshState(bool on)
{
  btnLibrary->setChecked(on);
  //   if (...............kapp->sessionSaving()) {
  //     // if we're saving the session, remember that history browser is on
  //   } else {
  //     _libraryBrowserIsShown = on;
  //   }
}


void KLFMainWin::insertSymbol(const KLFLatexSymbol& s)
{
  QTextCursor c1(txtLatex->document());
  c1.beginEditBlock();
  c1.setPosition(txtLatex->textCursor().position());
  c1.insertText(s.symbol+"\n");
  c1.deletePreviousChar(); // small hack for forcing a separate undo command when two symbols are inserted
  c1.endEditBlock();
  // see if we need to insert the xtrapreamble
  QStringList cmds = s.preamble;
  int k;
  QString preambletext = txtPreamble->toPlainText();
  QString addtext;
  QTextCursor c = txtPreamble->textCursor();
  c.beginEditBlock();
  c.movePosition(QTextCursor::End);
  for (k = 0; k < cmds.size(); ++k) {
    if (preambletext.indexOf(cmds[k]) == -1) {
      addtext = "";
      if (preambletext.length() > 0 &&
	  preambletext[preambletext.length()-1] != '\n')
	addtext = "\n";
      addtext += cmds[k];
      c.insertText(addtext);
      preambletext += addtext;
    }
  }
  c.endEditBlock();

  activateWindow();
  raise();
  txtLatex->setFocus();
}

void KLFMainWin::slotSymbolsButtonRefreshState(bool on)
{
  btnSymbols->setChecked(on);
  //   if (...................kapp->sessionSaving()) {
  //     // if we're saving the session, remember that latex symbols is on
  //   } else {
  //     _latexSymbolsIsShown = on;
  //   }
}

bool KLFMainWin::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == txtLatex) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      if (ke->key() == Qt::Key_Return && (QApplication::keyboardModifiers() == Qt::ShiftModifier ||
					  QApplication::keyboardModifiers() == Qt::ControlModifier)) {
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
  return QWidget::eventFilter(obj, e);
}


void KLFMainWin::hideEvent(QHideEvent *e)
{
  if (e->spontaneous())
    return;

  if (_ignore_hide_event)
    return;

  if (mLibraryBrowser)
    mLibraryBrowser->hide();
  if (mLatexSymbols)
    mLatexSymbols->hide();
  if (mStyleManager)
    mStyleManager->hide();
  if (mSettingsDialog)
    mSettingsDialog->hide();
  qApp->quit();
}


void KLFMainWin::alterSetting(altersetting_which which, int ivalue)
{
  _settings_altered = true;
  switch (which) {
  case altersetting_LBorderOffset:
    _settings.lborderoffset = ivalue; break;
  case altersetting_TBorderOffset:
    _settings.tborderoffset = ivalue; break;
  case altersetting_RBorderOffset:
    _settings.rborderoffset = ivalue; break;
  case altersetting_BBorderOffset:
    _settings.bborderoffset = ivalue; break;
  default:
    break;
  }
}
void KLFMainWin::alterSetting(altersetting_which which, QString svalue)
{
  _settings_altered = true;
  switch (which) {
  case altersetting_TempDir:
    _settings.tempdir = svalue; break;
  case altersetting_Latex:
    _settings.latexexec = svalue; break;
  case altersetting_Dvips:
    _settings.dvipsexec = svalue; break;
  case altersetting_Gs:
    _settings.gsexec = svalue; break;
  case altersetting_Epstopdf:
    _settings.epstopdfexec = svalue; break;
  default:
    break;
  }
}


bool KLFMainWin::importLibraryFileSeparateResources(const QString& fname, const QString& basername)
{
  return mLibraryBrowser->importLibraryFileSeparateResources(fname, basername);
}



void KLFMainWin::applySettings(const KLFBackend::klfSettings& s)
{
  _settings = s;
  _settings_altered = false;
}

void KLFMainWin::displayError(const QString& error)
{
  QMessageBox::critical(this, tr("Error"), error);
}


void KLFMainWin::updatePreviewBuilderThreadInput()
{
  bool reallyinputchanged = mPreviewBuilderThread->inputChanged(collectInput());
  if (reallyinputchanged) {
    _evaloutput_uptodate = false;
  }
}

void KLFMainWin::showRealTimePreview(const QImage& preview, bool latexerror)
{
  if (_evaloutput_uptodate)
    return;
  if (latexerror) {
    lblOutput->setStyleSheet("background-color: rgb(255, 220, 220)");
    return;
  } else {
    lblOutput->setStyleSheet("");
  }
  lblOutput->setPixmap(QPixmap::fromImage(preview));
  lblOutput->setEnabled(false);
}

KLFBackend::klfInput KLFMainWin::collectInput()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  input.latex = txtLatex->toPlainText();
  if (chkMathMode->isChecked()) {
    input.mathmode = cbxMathMode->currentText();
    if (cbxMathMode->findText(input.mathmode) == -1) {
      cbxMathMode->addItem(input.mathmode);
    }
  } else {
    input.mathmode = "...";
  }
  input.preamble = txtPreamble->toPlainText();
  input.fg_color = colFg->color().rgb();
  if (chkBgTransparent->isChecked() == false)
    input.bg_color = colBg->color().rgb();
  else
    input.bg_color = qRgba(255, 255, 255, 0);

  input.dpi = spnDPI->value();

  return input;
}

void KLFMainWin::slotEvaluate()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  btnEvaluate->setEnabled(false); // don't allow user to click us while we're not done, and
  //				     additionally this gives visual feedback to the user

  input = collectInput();

  // and GO !
  _output = KLFBackend::getLatexFormula(input, _settings);
  _lastrun_input = input;

  if (_output.status < 0) {
    QString comment = "";
    if (_output.status == -4) {
      comment = "\n"+tr("Are you sure you configured your system paths correctly in the settings dialog ?");
    }
    QMessageBox::critical(this, tr("Error"), _output.errorstr+comment);
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
    _evaloutput_uptodate = true;

    QPixmap sc;
    if (_output.result.width() > lblOutput->width() || _output.result.height() > lblOutput->height())
      sc = QPixmap::fromImage(_output.result.scaled(lblOutput->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
      sc = QPixmap::fromImage(_output.result);
    lblOutput->setPixmap(sc);
    lblOutput->setStyleSheet("");
    lblOutput->setEnabled(true);

    frmOutput->setEnabled(true);

    KLFData::KLFLibraryItem jj = { KLFData::KLFLibraryItem::MaxId++, QDateTime::currentDateTime(), input.latex, sc,
				   KLFData::categoryFromLatex(input.latex), KLFData::tagsFromLatex(input.latex),
				   KLFData::KLFStyle() };
    jj.style = currentStyle();

    if ( _library[_libresources[0]].size() == 0 ||
	 ! (_library[_libresources[0]].last() == jj) ) {
      mLibraryBrowser->addToHistory(jj);
    }

    QSize goodsize = _output.result.size();
    QImage img = _output.result;
    if ( klfconfig.UI.previewTooltipMaxSize != QSize(0, 0) && // QSize(0,0) meaning no resize
	 ( img.width() > klfconfig.UI.previewTooltipMaxSize.width() ||
	   img.height() > klfconfig.UI.previewTooltipMaxSize.height() ) ) {
      img = _output.result.scaled(klfconfig.UI.previewTooltipMaxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (mLastRunTempPNGFile) {
      delete mLastRunTempPNGFile;
    }
    mLastRunTempPNGFile = new QTemporaryFile(/*QDir::toNativeSeparators(klfconfig.BackendSettings.tempDir
                                                                      +"/klf_lastruntemp_XXXXXX.png"),*/
                                             this);

    if ( ! mLastRunTempPNGFile->open() ) {
      qWarning("WARNING: Failed open for Tooltip Temp Image!\n%s\n",
	       qPrintable(mLastRunTempPNGFile->fileTemplate()));
      QMessageBox::critical(this, tr("Error"), tr("Failed open for ToolTip Temp Image!\n%1")
			    .arg(mLastRunTempPNGFile->fileTemplate()));
      delete mLastRunTempPNGFile;
      mLastRunTempPNGFile = 0;
    } else {
      mLastRunTempPNGFile->setAutoRemove(true);
      bool res = img.save(mLastRunTempPNGFile, "PNG");
      if ( ! res ) {
        QMessageBox::critical(this, tr("Error"), tr("Failed write to ToolTip Temp Image file %1!")
			      .arg(mLastRunTempPNGFile->fileName()));
	qWarning("WARNING: Failed write to Tooltip temp image to temporary file `%s' !\n",
		 qPrintable(mLastRunTempPNGFile->fileTemplate()));
	delete mLastRunTempPNGFile;
	mLastRunTempPNGFile = 0;
      } else {
	if (klfconfig.UI.enableToolTipPreview)
	  lblOutput->setToolTip(QString("<qt><img src=\"%1\"></qt>").arg(mLastRunTempPNGFile->fileName()));
	else
	  lblOutput->setToolTip(QString(""));
      }
    }
  }

  btnEvaluate->setEnabled(true); // re-enable our button

  // and save our history and styles
  //  saveLibrary(); // save library is time-consuming. do it only on program close.
  //  saveStyles();

}

void KLFMainWin::slotClear()
{
  txtLatex->setPlainText("");
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
    //    adjustSize();
    btnExpand->setIcon(QIcon(":/pics/switchexpanded.png"));
  } else {
    setFixedSize(_expandedsize);
    frmDetails->show();
    //    adjustSize();
    btnExpand->setIcon(QIcon(":/pics/switchshrinked.png"));
  }
}

void KLFMainWin::slotSetLatex(const QString& latex)
{
  txtLatex->setPlainText(latex);
}

void KLFMainWin::slotSetMathMode(const QString& mathmode)
{
  chkMathMode->setChecked(mathmode.simplified() != "...");
  if (mathmode.simplified() != "...")
    cbxMathMode->setEditText(mathmode);
}

void KLFMainWin::slotSetPreamble(const QString& preamble)
{
  txtPreamble->setPlainText(preamble);
}

void KLFMainWin::slotSetDPI(int DPI)
{
  spnDPI->setValue(DPI);
}

void KLFMainWin::slotSetFgColor(const QColor& fg)
{
  colFg->setColor(fg);
}
void KLFMainWin::slotSetFgColor(const QString& s)
{
  QColor fgcolor;
  fgcolor.setNamedColor(s);
  slotSetFgColor(fgcolor);
}
void KLFMainWin::slotSetBgColor(const QColor& bg)
{
  colBg->setColor(bg);
}
void KLFMainWin::slotSetBgColor(const QString& s)
{
  QColor bgcolor;
  if (s == "-")
    bgcolor.setRgb(255, 255, 255, 0); // white transparent
  else
    bgcolor.setNamedColor(s);
  slotSetBgColor(bgcolor);
}

// function imported from main.cpp
extern void main_save(KLFBackend::klfOutput klfoutput, const QString& f_output, QString format);

void KLFMainWin::slotEvaluateAndSave(const QString& output, const QString& format)
{
  if ( txtLatex->toPlainText().isEmpty() )
    return;

  slotEvaluate();

  if ( ! output.isEmpty() ) {
    if ( _output.result.isNull() ) {
      QMessageBox::critical(this, tr("Error"), tr("There is no image to save."));
    } else {
      main_save(_output, output, format.trimmed().toUpper());
    }
  }

}

void KLFMainWin::importCmdlKLFFiles(const QStringList& files)
{
  int k;
  bool imported = false;
  for (k = 0; k < files.size(); ++k) {
    importLibraryFileSeparateResources(files[k], QFileInfo(files[k]).baseName() + ":");
    imported = true;
  }
  if (imported)
    slotLibrary(true);
}


void KLFMainWin::slotDrag()
{
  if ( _output.result.isNull() )
    return;

  QDrag *drag = new QDrag(this);
  QMimeData *mime = new QMimeData;
  mime->setImageData(_output.result);
  drag->setMimeData(mime);
  QImage img;
  img = _output.result.scaled(QSize(200, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap p = QPixmap::fromImage(img);
  drag->setPixmap(p);
  drag->setDragCursor(p, Qt::MoveAction);
  drag->setHotSpot(QPoint(p.width()/2, p.height()));
  drag->exec();
}
void KLFMainWin::slotCopy()
{
  QApplication::clipboard()->setImage(_output.result, QClipboard::Clipboard);
}

void KLFMainWin::slotSave(const QString& suggestfname)
{
  // application-long persistent selectedfilter
  static QString selectedfilter;

  QStringList formatlist, filterformatlist;
  QMap<QString,QString> formatsByFilterName;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();

  for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
    QString f = (*it).toLower();
    if (f == "jpg" || f == "jpeg" || f == "png" || f == "pdf" || f == "eps")
      continue;
    QString s = tr("%1 Image (*.%2)").arg(f).arg(f);
    filterformatlist << s;
    formatlist.push_back(f);
    formatsByFilterName[s] = f;
  }

  QString s;
  s = tr("EPS PostScript (*.eps)");
  filterformatlist.push_front(s);
  formatlist.push_front("eps");
  formatsByFilterName[s] = "eps";
  s = tr("PDF Portable Document Format (*.pdf)");
  filterformatlist.push_front(s);
  formatlist.push_front("pdf");
  formatsByFilterName[s] = "pdf";
  s = tr("Standard JPEG Image (*.jpg *.jpeg)");
  filterformatlist.push_front(s);
  formatlist.push_front("jpeg");
  formatlist.push_front("jpg");
  formatsByFilterName[s] = "jpeg";
  selectedfilter = s = tr("Standard PNG Image (*.png)");
  filterformatlist.push_front(s);
  formatlist.push_front("png");
  formatsByFilterName[s] = "png";

  QString filterformat = filterformatlist.join(";;");
  QString fname, format;
  QString suggestion = suggestfname;
  if (suggestion.isEmpty())
    suggestion = klfconfig.UI.lastSaveDir;
  fname = QFileDialog::getSaveFileName(this, tr("Save Image Formula"), suggestion, filterformat, &selectedfilter);

  if (fname.isEmpty())
    return;

  QFileInfo fi(fname);
  klfconfig.UI.lastSaveDir = fi.absolutePath();
  if ( fi.suffix().length() == 0 ) {
    // get format and suffix from selected filter
    if ( ! formatsByFilterName.contains(selectedfilter) ) {
      qWarning("ERROR: Unknown format filter selected: `%s'! Assuming PNG!\n", qPrintable(selectedfilter));
      format = "png";
    } else {
      format = formatsByFilterName[selectedfilter];
    }
    fname += "."+format;
    // !! : fname has changed, potentially the file could exist, user would not have been warned.
    if (QFile::exists(fname)) {
      int r = QMessageBox::warning(this, tr("File Exists"),
				   tr("The file <b>%1</b> already exists.\nOverwrite?").arg(fname),
				   QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::No);
      if (r == QMessageBox::Yes) {
	// will continue in this function.
      } else if (r == QMessageBox::No) {
	// re-prompt for file name & save (by recursion), and return
	slotSave(fname);
	return;
      } else {
	// cancel
	return;
      }
    }
  }
  fi.setFile(fname);
  int index = formatlist.indexOf(fi.suffix());
  if ( index < 0 ) {
    // select PNG by default if suffix is not recognized
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::Warning);
    msgbox.setWindowTitle(tr("Extension not recognized"));
    msgbox.setText(tr("Extension <b>%1</b> not recognized.").arg(fi.suffix()));
    msgbox.setInformativeText(tr("Press \"Change\" to change the file name, or \"Use PNG\" to save as PNG."));
    QPushButton *png = new QPushButton(tr("Use PNG"), &msgbox);
    msgbox.addButton(png, QMessageBox::AcceptRole);
    QPushButton *chg  = new QPushButton(tr("Change ..."), &msgbox);
    msgbox.addButton(chg, QMessageBox::ActionRole);
    QPushButton *cancel = new QPushButton(tr("Cancel"), &msgbox);
    msgbox.addButton(cancel, QMessageBox::RejectRole);
    msgbox.setDefaultButton(chg);
    msgbox.setEscapeButton(cancel);
    msgbox.exec();
    if (msgbox.clickedButton() == png) {
      format = "png";
    } else if (msgbox.clickedButton() == cancel) {
      return;
    } else {
      // re-prompt for file name & save (by recursion), and return
      slotSave(fname);
      return;
    }
  } else {
    format = formatlist[index];
  }

  // The Qt dialog, or us, already asked user to confirm overwriting existing files

  QByteArray *dataptr = 0;
  if (format == "ps" || format == "eps") {
    dataptr = &_output.epsdata;
  } else if (format == "pdf") {
    dataptr = &_output.pdfdata;
  } else if (format == "png") {
    // we want to add meta-info in PNG image...
    //dataptr = &_output.pngdata;
    dataptr = 0;
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
    fsav.write(*dataptr);
  } else {
    // add text information for latex formula, style, etc.
    // with QImageWriter
    QImageWriter writer(fname, format.toUpper().toLatin1());
    writer.setQuality(90);
    writer.setText("Application", tr("Created with KLatexFormula version %1").arg(QString::fromAscii(version)));
    writer.setText("AppVersion", QString::fromLatin1("KLF ")+version);
    writer.setText("InputLatex", _lastrun_input.latex);
    writer.setText("InputMathMode", _lastrun_input.mathmode);
    writer.setText("InputPreamble", _lastrun_input.preamble);
    writer.setText("InputFgColor", QString("rgb(%1, %2, %3)").arg(qRed(_lastrun_input.fg_color))
		   .arg(qGreen(_lastrun_input.fg_color)).arg(qBlue(_lastrun_input.fg_color)));
    writer.setText("InputBgColor", QString("rgba(%1, %2, %3, %4)").arg(qRed(_lastrun_input.bg_color))
		   .arg(qGreen(_lastrun_input.bg_color)).arg(qBlue(_lastrun_input.bg_color))
		   .arg(qAlpha(_lastrun_input.bg_color)));
    writer.setText("InputDPI", QString::number(_lastrun_input.dpi));

    //    _output.result.save(fname, format.toUpper().toLocal8Bit().constData());

    bool res = writer.write(_output.result);
    if ( ! res ) {
      QMessageBox::critical(this, tr("Error"), writer.errorString());
      return;
    }
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
  sty.preamble = txtPreamble->toPlainText();
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
  if (_styles[n].mathmode.trimmed() == "...") {
    chkMathMode->setChecked(false);
  } else {
    chkMathMode->setChecked(true);
    cbxMathMode->setEditText(_styles[n].mathmode);
  }
  txtPreamble->setPlainText(_styles[n].preamble);
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
  for (int kl = 0; found_i == -1 && kl < _styles.size(); ++kl) {
    if (_styles[kl].name.trimmed() == name.trimmed()) {
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
  mSettingsDialog->show();
}


void KLFMainWin::closeEvent(QCloseEvent *event)
{
  event->accept();
}





//.......................................
/*
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
*/

