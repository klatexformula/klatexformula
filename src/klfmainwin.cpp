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

#include <qcombobox.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qimage.h>
#include <qpushbutton.h>
#include <qtextbrowser.h>
#include <qfile.h>
#include <qfont.h>
#include <qclipboard.h>
#include <qdragobject.h>
#include <qsyntaxhighlighter.h>
#include <qtooltip.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kmainwindow.h>
#include <kcolorcombo.h>
#include <kcombobox.h>
#include <ktextedit.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <khelpmenu.h>
#include <kaboutdata.h>

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

// KLatexFormula "About" Data
extern KAboutData *klfaboutdata;


/* This is for backwards compatibility with older versions than 2.0.0
 * These classes and functions are only used in loadLibrary() and the loaded stuff is directly
 * converted to the new formats. */
/* ---------------------------------------------- */
/** A small class to store info on the context
 * of evaluation of a formula */
struct KLFOldHistContext {
  bool changecolors;
  QColor fgcolor;
  QColor bgcolor;
  bool bgtransparent;
  int imgborderoffset;
  int imgdpi;
  bool latexmathmode;
  QString latexmathmodedelimiters;
  QString latexpreamble;
};

struct KLFOldHistoryElement {
  QDateTime tm;
  QPixmap preview;
  QString latexstring;

  Q_INT32 id;

  KLFOldHistContext context;

  static Q_INT32 max_id;
};

Q_INT32 KLFOldHistoryElement::max_id;

QDataStream& operator>>(QDataStream& str, KLFOldHistContext& c)
{
  Q_INT8 i_changecolors, i_bgtransparent, i_latexmathmode;
  Q_INT32 i_imgborderoffset, i_imgdpi;
  str >> i_changecolors >> c.fgcolor >> c.bgcolor >> i_bgtransparent >> i_imgborderoffset >> i_imgdpi
      >> i_latexmathmode >> c.latexmathmodedelimiters >> c.latexpreamble;
  c.changecolors = (bool)i_changecolors;
  c.bgtransparent = (bool)i_bgtransparent;
  c.latexmathmode = (bool)i_latexmathmode;
  c.imgborderoffset = i_imgborderoffset;
  c.imgdpi = i_imgdpi;

  return str;
}
QDataStream& operator>>(QDataStream& str, KLFOldHistoryElement& item)
{
  str >> item.id;
  str >> item.tm;
  str >> item.preview;
  str >> item.latexstring;
  str >> item.context;
  return str;
}

// the list type
typedef QValueList<KLFOldHistoryElement> KLFOldHistory;

/* ------------------------------------------------ */




KLFLatexSyntaxHighlighter::KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent)
  : QObject(parent), QSyntaxHighlighter(textedit)
{
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
  textEdit()->getCursorPosition(&pa, &po);
  setCaretPos(pa, po);
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

  btnClose->setIconSet(QIconSet(locate("appdata", "pics/closehide.png")));
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
 : KMainWindow()
{
  mMainWidget = new KLFMainWinUI(this);

  loadSettings();

  _output.status = 0;
  _output.errorstr = QString();
  _output.result = QImage();
  _output.pngdata = QByteArray();
  _output.epsdata = QByteArray();
  _output.pdfdata = QByteArray();

  // load styles
  loadStyles();
  // load library
  loadLibrary();

  _libraryBrowserIsShown = false;
  _latexSymbolsIsShown = false;

  mLibraryBrowser = new KLFLibraryBrowser(&_library, &_libresources, this); // Don't pass NULL here

  mLatexSymbols = new KLFLatexSymbols(this);


  // setup mMainWidget UI correctly

  QToolTip::add(mMainWidget->lblPromptMain, i18n("KLatexFormula %1").arg(QString("")+version));

  mMainWidget->btnQuit->setIconSet(QIconSet(locate("appdata", "pics/closehide.png")));
  mMainWidget->btnClear->setPixmap(locate("appdata", "pics/clear.png"));
  mMainWidget->btnExpand->setPixmap(locate("appdata", "pics/switchexpanded.png"));
  mMainWidget->btnEvaluate->setIconSet(QPixmap(locate("appdata", "pics/evaluate.png")));
  mMainWidget->btnEvaluate->setText(i18n("Evaluate"));
  mMainWidget->btnLibrary->setPixmap(locate("appdata", "pics/library.png"));
  mMainWidget->btnSymbols->setPixmap(locate("appdata", "pics/symbols.png"));
  mMainWidget->btnLoadStyle->setIconSet(QPixmap(locate("appdata", "pics/loadstyle.png")));
  mMainWidget->btnSaveStyle->setIconSet(QPixmap(locate("appdata", "pics/savestyle.png")));
  mMainWidget->btnSettings->setIconSet(QPixmap(locate("appdata", "pics/settings.png")));
  mMainWidget->btnHelp->setIconSet(QPixmap(locate("appdata", "pics/help.png")));
  mMainWidget->btnHelp->setText("");

  mMainWidget->txeLatex->setFont(klfconfig.Appearance.latexEditFont);

  QFont font = mMainWidget->btnDrag->font();
  font.setPointSize(font.pointSize()-2);
  mMainWidget->btnDrag->setFont(font);
  mMainWidget->btnCopy->setFont(font);
  mMainWidget->btnSave->setFont(font);
  font = mMainWidget->txePreamble->font();
  font.setPointSize(font.pointSize()-2);
  mMainWidget->txePreamble->setFont(font);

  mMainWidget->frmOutput->setEnabled(false);

  mMainWidget->txeLatex->setHScrollBarMode(QScrollView::Auto);
  mMainWidget->txeLatex->setVScrollBarMode(QScrollView::Auto);

  mHighlighter = new KLFLatexSyntaxHighlighter(mMainWidget->txeLatex);
  mPreambleHighlighter = new KLFLatexSyntaxHighlighter(mMainWidget->txePreamble);

  connect(mMainWidget->txeLatex, SIGNAL(cursorPositionChanged(int, int)),
	  this, SLOT(refreshCaretPosition(int, int)));
  connect(mMainWidget->txePreamble, SIGNAL(cursorPositionChanged(int, int)),
	  this, SLOT(refreshPreambleCaretPosition(int, int)));

  _shrinkedsize = mMainWidget->frmMain->sizeHint() + QSize(3, 3);
  _expandedsize.setWidth(mMainWidget->frmMain->sizeHint().width()
			 + mMainWidget->frmDetails->sizeHint().width() + 3);
  _expandedsize.setHeight(mMainWidget->frmMain->sizeHint().height() + 3);

  mMainWidget->lblOutput->setFixedSize(klfconfig.Appearance.labelOutputFixedSize);

  mMainWidget->frmOutput->layout()->invalidate();

  KHelpMenu *helpMenu = new KHelpMenu(this, klfaboutdata);
  mMainWidget->btnHelp->setPopup(helpMenu->menu());

  mMainWidget->frmDetails->hide();

  mMainWidget->frmOutput->setFixedHeight(mMainWidget->frmOutput->minimumSizeHint().height());

  setCentralWidget(mMainWidget);

  mMainWidget->lblOutput->installEventFilter(this);
  mMainWidget->txeLatex->installEventFilter(this);

  setFixedSize(_shrinkedsize);

  // Create our style manager
  mStyleManager = new KLFStyleManager(&_styles, this);
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(refreshStylePopupMenus()));
  connect(this, SIGNAL(stylesChanged()), mStyleManager, SLOT(stylesChanged()));

  // load style "default" or "Default" if one of them exists
  loadNamedStyle("default");
  loadNamedStyle("Default");

  // For systematical syntax highlighting
  // make sure syntax highlighting is up-to-date at all times
  QTimer *synthighlighttimer = new QTimer(this);
  synthighlighttimer->start(200);
  connect(synthighlighttimer, SIGNAL(timeout()), this, SLOT(refreshSyntaxHighlighting()));
  connect(synthighlighttimer, SIGNAL(timeout()), this, SLOT(refreshPreambleSyntaxHighlighting()));


  connect(mMainWidget->btnClear, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(mMainWidget->btnEvaluate, SIGNAL(clicked()), this, SLOT(slotEvaluate()));
  connect(mMainWidget->btnSymbols, SIGNAL(toggled(bool)), this, SLOT(slotSymbols(bool)));
  connect(mMainWidget->btnLibrary, SIGNAL(toggled(bool)), this, SLOT(slotLibrary(bool)));
  connect(mMainWidget->btnExpand, SIGNAL(clicked()), this, SLOT(slotExpandOrShrink()));
  connect(mMainWidget->btnCopy, SIGNAL(clicked()), this, SLOT(slotCopy()));
  connect(mMainWidget->btnDrag, SIGNAL(released()), this, SLOT(slotDrag()));
  connect(mMainWidget->btnSave, SIGNAL(clicked()), this, SLOT(slotSave()));
  connect(mMainWidget->btnSettings, SIGNAL(clicked()), this, SLOT(slotSettings()));
  connect(mMainWidget->btnQuit, SIGNAL(clicked()), this, SLOT(close()));
  connect(mStyleMenu, SIGNAL(activated(int)), this, SLOT(slotLoadStyle(int)));
  connect(mMainWidget->btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  //  connect(mMainWidget->txeLatex, SIGNAL(textChanged()), this, SLOT(refreshSyntaxHighlighting()));

  connect(mLibraryBrowser, SIGNAL(restoreFromLibrary(KLFData::KLFLibraryItem, bool)),
	  this, SLOT(restoreFromLibrary(KLFData::KLFLibraryItem, bool)));
  connect(mLibraryBrowser, SIGNAL(refreshLibraryBrowserShownState(bool)),
	  this, SLOT(slotLibraryButtonRefreshState(bool)));
  connect(mLatexSymbols, SIGNAL(insertSymbol(QString,QString)), this, SLOT(insertSymbol(QString,QString)));
  connect(mLatexSymbols, SIGNAL(refreshSymbolBrowserShownState(bool)),
	  this, SLOT(slotSymbolsButtonRefreshState(bool)));

  connect(this, SIGNAL(stylesChanged()), this, SLOT(saveStyles()));
  connect(mStyleManager, SIGNAL(refreshStyles()), this, SLOT(saveStyles()));

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
  _settings.klfclspath = locate("appdata", "klatexformula.cls");
  _settings.klfclspath = _settings.klfclspath.left(_settings.klfclspath.findRev('/'));

  _settings.tempdir = klfconfig.BackendSettings.tempDir;
  _settings.latexexec = klfconfig.BackendSettings.execLatex;
  _settings.dvipsexec = klfconfig.BackendSettings.execDvips;
  _settings.gsexec = klfconfig.BackendSettings.execGs;
  _settings.epstopdfexec = klfconfig.BackendSettings.execEpstopdf;

  _settings.lborderoffset = klfconfig.BackendSettings.lborderoffset;
  _settings.tborderoffset = klfconfig.BackendSettings.tborderoffset;
  _settings.rborderoffset = klfconfig.BackendSettings.rborderoffset;
  _settings.bborderoffset = klfconfig.BackendSettings.bborderoffset;

}

void KLFMainWin::saveSettings()
{
  klfconfig.writeToConfig();
}

void KLFMainWin::refreshSyntaxHighlighting()
{
  //   int pa, po;
  //   mMainWidget->txeLatex->getCursorPosition(&pa, &po);
  //   mHighlighter->setCaretPos(pa, po);
  //   mHighlighter->rehighlight();
  mHighlighter->refreshAll();
}

void KLFMainWin::refreshCaretPosition(int /*para*/, int /*pos*/)
{
  //  mHighlighter->setCaretPos(para, pos); // will automatically refresh
  refreshSyntaxHighlighting();
}

void KLFMainWin::refreshPreambleSyntaxHighlighting()
{
  //   int pa, po;
  //   mMainWidget->txePreamble->getCursorPosition(&pa, &po);
  //   mPreambleHighlighter->setCaretPos(pa, po);
  //   mPreambleHighlighter->rehighlight();
  mPreambleHighlighter->refreshAll();
}

void KLFMainWin::refreshPreambleCaretPosition(int /*para*/, int /*pos*/)
{
  //  mHighlighter->setCaretPos(para, pos); // will automatically refresh
  refreshPreambleSyntaxHighlighting();
}

void KLFMainWin::refreshStylePopupMenus()
{
  if (!mStyleMenu)
    mStyleMenu = new KPopupMenu(this);
  mStyleMenu->clear();

  mStyleMenu->insertTitle(i18n("Available Styles"), 100000);
  for (uint i = 0; i < _styles.size(); ++i) {
    mStyleMenu->insertItem(_styles[i].name, i);
  }
  mStyleMenu->insertSeparator();
  mStyleMenu->insertItem(QIconSet(locate("appdata", "pics/managestyles.png")), i18n("Manage Styles"),
			 this, SLOT(slotStyleManager()), 0 /* accel */, 100001 /* id */);
}
void KLFMainWin::loadStyles()
{
  _styles = KLFData::KLFStyleList(); // empty list by default
  if (QFile::exists(locate("appdata", "styles"))) {
    QFile fsty(locate("appdata", "styles"));
    if ( ! fsty.open(IO_ReadOnly|IO_Raw) )
      KMessageBox::error(this, i18n("Error: Unable to load your style list!"), i18n("Error"));
    else {
      QDataStream str(&fsty);
      QString s1;
      str >> s1;
      // check for KLATEXFORMULA_STYLE_LIST
      if (s1 != "KLATEXFORMULA_STYLE_LIST") {
	KMessageBox::error(this, i18n("Error: Style file is incorrect or corrupt!\n"), i18n("Error"));
      } else {
	Q_INT16 vmaj, vmin;
	str >> vmaj >> vmin;

	if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	  KMessageBox::information(this, i18n("The style file found was created by a more recent version of KLatexFormula.\n"
					      "The process of style loading may fail."),  i18n("Load styles"));
	}

	str >> _styles;

      } // bad file format
    }
  } // file exists

  if (_styles.isEmpty()) {
    // if stylelist is empty, populate a few default styles
    KLFData::KLFStyle s1 = { i18n("Default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "\\usepackage{amssymb,amsmath}\n", 1200 };
    _styles.append(s1);
    /*
     KLFData::KLFStyle s2 = { i18n("small, white bg"), qRgb(0, 0, 0), qRgba(255, 255, 255, 255), "\\[ ... \\]", "", 150 };
     _styles.append(s2);
     KLFData::KLFStyle s3 = { i18n("no math"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "...", "", 1200 };
     _styles.append(s3);
    */
  }

  mStyleMenu = 0;
  refreshStylePopupMenus();

  mMainWidget->btnLoadStyle->setPopup(mStyleMenu);
}

void KLFMainWin::saveStyles()
{
  QString s = locateLocal("appdata", "styles");
  if (s.isEmpty()) {
    KMessageBox::error(this, i18n("Error: Unable to make local file for saving styles!\n"), i18n("Error"));
    return;
  }
  QFile f(s);
  if ( ! f.open(IO_WriteOnly | IO_Raw) ) {
    KMessageBox::error(this, i18n("Error: Unable to write to styles file!\n"), i18n("Error"));
    return;
  }
  QDataStream stream(&f);
  stream << QString("KLATEXFORMULA_STYLE_LIST") << (Q_INT16)version_maj << (Q_INT16)version_min << _styles;
}

void KLFMainWin::loadLibrary()
{
  _library = KLFData::KLFLibrary();
  _libresources = KLFData::KLFLibraryResourceList();

  KLFData::KLFLibraryResource hist = { KLFData::LibResource_History, i18n("History") };
  _libresources.append(hist);
  KLFData::KLFLibraryResource archive = { KLFData::LibResource_Archive, i18n("Archive") };
  _libresources.append(archive);


  QString fname = locate("appdata", "library");
  if ( ! fname.isEmpty() ) {
    QFile flib(fname);
    if ( ! flib.open(IO_ReadOnly | IO_Raw) ) {
      KMessageBox::error(this, i18n("Unable to open library!"), i18n("Error"));
    } else {
      QDataStream stream(&flib);
      QString s1;
      stream >> s1;
      if (s1 != "KLATEXFORMULA_LIBRARY") {
	KMessageBox::error(this, i18n("Error: Library file is incorrect or corrupt!\n"), i18n("Error"));
      } else {
	Q_INT16 vmaj, vmin;
	stream >> vmaj >> vmin;

	if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	  KMessageBox::information(this, i18n("The history file found was created by a more recent version of KLatexFormula.\n"
					      "The process of history loading may fail."),  i18n("Load history"));
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
      if ( ! fhist.open(IO_ReadOnly | IO_Raw) ) {
	KMessageBox::error(this, i18n("Unable to load your formula history list!"), i18n("Error"));
      } else {
	QDataStream stream(&fhist);
	QString s1;
	stream >> s1;
	if (s1 != "KLATEXFORMULA_HISTORY") {
	  KMessageBox::error(this, i18n("Error: History file is incorrect or corrupt!\n"), i18n("Error"));
	} else {
	  Q_INT16 vmaj, vmin;
	  stream >> vmaj >> vmin;
  
	  if (vmaj > version_maj || (vmaj == version_maj && vmin > version_min)) {
	    KMessageBox::information(this, i18n("The history file found was created by a more recent version of KLatexFormula.\n"
						"The process of history loading may fail."),  i18n("Load history"));
	  }

	  KLFData::KLFLibraryList history;
  
	  stream >> KLFData::KLFLibraryItem::MaxId >> history;

	  _library[_libresources[0]] = history;
  
	} // format corrupt
      } // open file ok
    } // file exists
    else {
      fname = locate("appdata", "klatexformula_history");
      if ( ! fname.isEmpty()) {
	int r = KMessageBox::questionYesNo(this, i18n("KLatexFormula has detected a history file from an older version of KLatexFormula.\n"
						      "Do you wish to import those items?"), i18n("Import old history?"));
	if (r == KMessageBox::Yes) {
	  QFile fh(fname);
	  if ( ! fh.open(IO_ReadOnly) ) {
	    KMessageBox::error(this, i18n("Unable to load your old formula history list!"), i18n("Error"));
	  } else {
	    QDataStream stream(&fh);
  
	    KLFOldHistory old_hist;

	    stream >> KLFOldHistoryElement::max_id >> old_hist;
  
	    // now convert old history to new history
  
	    KLFData::KLFLibraryItem::MaxId = KLFOldHistoryElement::max_id+1; // just to be sure
  
	    for (uint i = 0; i < old_hist.size(); ++i) {
	      KLFData::KLFLibraryItem h;
	      h.id = old_hist[i].id;
	      h.datetime = old_hist[i].tm;
	      h.latex = old_hist[i].latexstring;
	      h.preview = old_hist[i].preview;
	      h.category = ""; // this is a dirty hypothesis that h.latex doens't start with '%:' !!
	      h.tags = ""; // this is a dirty hypothesis that h.latex doens't start with '%:'/'%' !!
	      h.style.name = i18n("[old style import]");
	      h.style.fg_color = old_hist[i].context.changecolors ? old_hist[i].context.fgcolor.rgb() : qRgb(0, 0, 0);
	      h.style.bg_color = old_hist[i].context.changecolors ?
		( (old_hist[i].context.bgtransparent) ? qRgba(255, 255, 255, 0) : old_hist[i].context.bgcolor.rgb() )
		: qRgba(255, 255, 255, 255); // remember: by default, in old versions BG was opaque
	      h.style.mathmode = old_hist[i].context.latexmathmode ? old_hist[i].context.latexmathmodedelimiters : "...";
	      h.style.preamble = old_hist[i].context.latexpreamble;
	      h.style.dpi = old_hist[i].context.imgdpi;
	      // old_hist[i].context.imgborderoffset is dropped because the concept is obsolete
  
	      _library[_libresources[0]].append(h);
	    }
	  }
	} // yes to import
      } // QFile::exists(oldhistoryfile)
    } // else-block of QFile::exists(newhistoryfile)

    /* <<<<<< END IMPORT PRE-2.1 HISTORY */
  }

  emit libraryAllChanged();
}

void KLFMainWin::saveLibrary()
{
  QString s = locateLocal("appdata", "library");
  if (s.isEmpty()) {
    KMessageBox::error(this, i18n("Error: Unable to locate local file for saving library!\n"), i18n("Error"));
    return;
  }
  QFile f(s);
  if ( ! f.open(IO_WriteOnly | IO_Raw) ) {
    KMessageBox::error(this, i18n("Error: Unable to write to library file!\n"), i18n("Error"));
    return;
  }
  QDataStream stream(&f);
  stream << QString("KLATEXFORMULA_LIBRARY") << (Q_INT16)version_maj << (Q_INT16)version_min
	 << KLFData::KLFLibraryItem::MaxId << _libresources << _library;
}

void KLFMainWin::restoreFromLibrary(KLFData::KLFLibraryItem j, bool restorestyle)
{
  mMainWidget->txeLatex->setText(j.latex);
  if (restorestyle) {
    mMainWidget->kccFg->setColor(QColor(qRed(j.style.fg_color), qGreen(j.style.fg_color), qBlue(j.style.fg_color)));
    mMainWidget->kccBg->setColor(QColor(qRed(j.style.bg_color), qGreen(j.style.bg_color), qBlue(j.style.bg_color)));
    mMainWidget->chkBgTransparent->setChecked(qAlpha(j.style.bg_color) == 0);
    mMainWidget->chkMathMode->setChecked(j.style.mathmode.simplifyWhiteSpace() != "...");
    if (j.style.mathmode.simplifyWhiteSpace() != "...")
      mMainWidget->cbxMathMode->setCurrentText(j.style.mathmode);
    mMainWidget->txePreamble->setText(j.style.preamble);
    mMainWidget->spnDPI->setValue(j.style.dpi);
  }

  mMainWidget->lblOutput->setPixmap(j.preview);
  mMainWidget->frmOutput->setEnabled(false);
}
 
void KLFMainWin::slotLibraryButtonRefreshState(bool on)
{
  mMainWidget->btnLibrary->setOn(on);
  if (kapp->sessionSaving()) {
    // if we're saving the session, remember that history browser is on
  } else {
    _libraryBrowserIsShown = on;
  }
}


void KLFMainWin::insertSymbol(QString s, QString xtrapreamble)
{
  mMainWidget->txeLatex->insert(s);
  // see if we need to insert the xtrapreamble
  QStringList cmds = QStringList::split("%%", xtrapreamble);
  uint k;
  QString preambletext = mMainWidget->txePreamble->text();
  for (k = 0; k < cmds.size(); ++k) {
    if (preambletext.find(cmds[k]) == -1) {
      if (preambletext[preambletext.length()-1] != '\n')
	preambletext += "\n";
      preambletext += cmds[k];
    }
  }
  mMainWidget->txePreamble->setText(preambletext);
}

void KLFMainWin::slotSymbolsButtonRefreshState(bool on)
{
  mMainWidget->btnSymbols->setOn(on);
  if (kapp->sessionSaving()) {
    // if we're saving the session, remember that latex symbols is on
  } else {
    _latexSymbolsIsShown = on;
  }
}

bool KLFMainWin::eventFilter(QObject *obj, QEvent *e)
{
  if (obj == mMainWidget->txeLatex) {
    if (e->type() == QEvent::KeyPress) {
      QKeyEvent *ke = (QKeyEvent*) e;
      if (ke->key() == Key_Return && (ke->state() == ShiftButton || ke->state() == ControlButton)) {
	slotEvaluate();
	return true;
      }
    }
  }
  if (obj == mMainWidget->lblOutput) {
    if (e->type() == QEvent::MouseMove) {
      slotDrag();
    }
  }
  return KMainWindow::eventFilter(obj, e);
}


void KLFMainWin::slotEvaluate()
{
  // KLFBackend input
  KLFBackend::klfInput input;

  mMainWidget->btnEvaluate->setEnabled(false); // don't allow user to click us while we're not done, plus
  //						  this gives visual feedback to the user

  input.latex = mMainWidget->txeLatex->text();
  if (mMainWidget->chkMathMode->isChecked()) {
    input.mathmode = mMainWidget->cbxMathMode->currentText();
    mMainWidget->cbxMathMode->addToHistory(input.mathmode);
  } else {
    input.mathmode = "...";
  }
  input.preamble = mMainWidget->txePreamble->text();
  input.fg_color = mMainWidget->kccFg->color().rgb();
  if (mMainWidget->chkBgTransparent->isChecked() == false)
    input.bg_color = mMainWidget->kccBg->color().rgb();
  else
    input.bg_color = qRgba(255, 255, 255, 0);

  input.dpi = mMainWidget->spnDPI->value();


  _output = KLFBackend::getLatexFormula(input, _settings);

  if (_output.status < 0) {
    KMessageBox::error(this, i18n("Error: %1").arg(_output.errorstr), i18n("Error"));
    mMainWidget->lblOutput->setText("");
    mMainWidget->frmOutput->setEnabled(false);
  }
  if (_output.status > 0) {
    KLFProgErr::showError(this, _output.errorstr);
    mMainWidget->frmOutput->setEnabled(false);
  }
  if (_output.status == 0) {
    // ALL OK

    QPixmap sc = _output.result.smoothScale(mMainWidget->lblOutput->size(), QImage::ScaleMin);
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
      img = _output.result.smoothScale(klfconfig.Appearance.previewTooltipMaxSize, QImage::ScaleMin);
    }

    QMimeSourceFactory::defaultFactory()->setImage( "klfoutput", img );
    QToolTip::add(mMainWidget->lblOutput, QString("<qt><img src=\"klfoutput\"></qt>"));
  }

  mMainWidget->btnEvaluate->setEnabled(true); // re-enable our button

  // and save our history and styles
  saveLibrary();
  saveStyles();

}

void KLFMainWin::slotClear()
{
  mMainWidget->txeLatex->setText("");
  mMainWidget->txeLatex->setFocus();
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
  if (mMainWidget->frmDetails->isVisible()) {
    mMainWidget->frmDetails->hide();
    setFixedSize(_shrinkedsize);
    mMainWidget->btnExpand->setPixmap(locate("appdata", "pics/switchexpanded.png"));
  } else {
    setFixedSize(_expandedsize);
    mMainWidget->frmDetails->show();
    mMainWidget->btnExpand->setPixmap(locate("appdata", "pics/switchshrinked.png"));
  }
}

void KLFMainWin::slotQuit()
{
  close();
}

void KLFMainWin::slotDrag()
{
  QImageDrag *drag = new QImageDrag(_output.result, this);
  //  drag->setImage(res);
  QPixmap p;
  p = _output.result.smoothScale(QSize(200, 100), QImage::ScaleMin);
  drag->setPixmap(p, QPoint(20, 10));
  drag->dragCopy();
}
void KLFMainWin::slotCopy()
{
  kapp->clipboard()->setImage(_output.result, QClipboard::Clipboard);
}
void KLFMainWin::slotSave()
{
  QString filterformat = "";
  QStringList formats = QImage::outputFormatList();

  for (QStringList::iterator it = formats.begin(); it != formats.end(); ++it) {
    if (!filterformat.isEmpty()) filterformat += "\n";
    filterformat += i18n("*.%1|%2 Image").arg((*it).lower()).arg((*it).lower());
    //  filterformat += "*."+(*it).lower()+"|"+(*it).lower()+" Image";
  }

  filterformat = i18n("*.eps|EPS PostScript\n")+filterformat;
  filterformat = i18n("*.pdf|PDF Portable Document Format\n")+filterformat;

  filterformat = i18n("*.jpg|Standard JPEG Image\n")+filterformat;
  filterformat = i18n("*.png|Standard PNG Image\n")+filterformat;

  QString fname, format;
  {
    KFileDialog dlg(":klatexformulasave", filterformat, this, "filedialog", true);
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption(i18n("Save Image Formula"));
    dlg.exec();
    fname = dlg.selectedFile();
    format = dlg.currentFilter();
    int ind = format.find(".");

    format = format.mid(ind+1, min_i(format.find("|", ind+2), format.find(";", ind+2)) - (ind+1));
    if (format == "jpg")
      format = "jpeg"; // Qt doesn't understand "jpg"
  }

  if (fname.isEmpty())
    return;

  if (QFile::exists(fname)) {
    int res = KMessageBox::questionYesNoCancel(this, i18n("Specified file exists. Overwrite?"), i18n("Overwrite?"));
    if (res == KMessageBox::No) {
      slotSave(); // recurse
      // and quit
      return;
    }
    if (res == KMessageBox::Cancel) {
      // quit directly
      return;
    }
    // if res == KMessageBox::Yes, we may continue ...
  }


  //  QString format = fname.mid(fname.findRev(".")+1).upper();
  QByteArray *dataptr = 0;
  if (format == "ps" || format == "eps") {
    dataptr = &_output.epsdata;
  } else if (format == "pdf") {
    dataptr = &_output.pdfdata;
  } else if (format == "png") {
    dataptr = &_output.pngdata;
  } else {
    dataptr = 0;
  }
  
  if (dataptr) {
    if (dataptr->size() == 0) {
      KMessageBox::error(this, i18n("Sorry, format `%1' is not available.").arg(format), i18n("No %1").arg(format));
      slotSave(); // recurse and prompt user.
      return;
    }
    QFile fsav(fname);
    if ( ! fsav.open(IO_WriteOnly | IO_Raw) ) {
      KMessageBox::error(this, i18n("Error: Can't write to file %1!").arg(fname));
      return;
    }
    //    for (int k = 0; k < dataptr->size(); ++k) fsav.putch(dataptr->at(k));
    fsav.writeBlock(*dataptr);
  } else {
    _output.result.save(fname, format.upper());
  }

}


KLFData::KLFStyle KLFMainWin::currentStyle() const
{
  KLFData::KLFStyle sty;

  sty.name = QString::null;
  sty.fg_color = mMainWidget->kccFg->color().rgb();
  QColor bgc = mMainWidget->kccBg->color();
  sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), mMainWidget->chkBgTransparent->isChecked() ? 0 : 255 );
  sty.mathmode = (mMainWidget->chkMathMode->isChecked() ? mMainWidget->cbxMathMode->currentText() : "...");
  sty.preamble = mMainWidget->txePreamble->text();
  sty.dpi = mMainWidget->spnDPI->value();

  return sty;
}

void KLFMainWin::slotLoadStyle(int n)
{
  if (n < 0 || n >= (int)_styles.size())
    return; // let's not try to load an inexistant style... this is useful for special items in menu too

  uint fg = _styles[n].fg_color;
  uint bg = _styles[n].bg_color;
  mMainWidget->kccFg->setColor(QColor(qRed(fg), qGreen(fg), qBlue(fg)));
  mMainWidget->kccBg->setColor(QColor(qRed(bg), qGreen(bg), qBlue(bg)));
  mMainWidget->chkBgTransparent->setChecked(qAlpha(bg) == 0);
  if (_styles[n].mathmode.simplifyWhiteSpace() == "...") {
    mMainWidget->chkMathMode->setChecked(false);
  } else {
    mMainWidget->chkMathMode->setChecked(true);
    mMainWidget->cbxMathMode->setCurrentText(_styles[n].mathmode);
  }
  mMainWidget->txePreamble->setText(_styles[n].preamble);
  mMainWidget->spnDPI->setValue(_styles[n].dpi);
}
void KLFMainWin::slotSaveStyle()
{
  KLFData::KLFStyle sty;

  QString name = KInputDialog::getText(i18n("Enter name"), i18n("Enter new style name: "), i18n("New Style"), 0, this);
  if (name.isEmpty()) {
    return;
  }

  // check to see if style exists already
  int found_i = -1;
  for (uint kl = 0; found_i == -1 && kl < _styles.size(); ++kl) {
    if (_styles[kl].name.stripWhiteSpace() == name.stripWhiteSpace()) {
      found_i = kl;
      // style exists already
      int r = KMessageBox::questionYesNoCancel(this, i18n("Style name already exists. Do you want to overwrite?"),  i18n("Style name exists"));
      switch (r) {
      case KMessageBox::No:
	slotSaveStyle(); // recurse, ask for new name.
	return; // and stop this function execution of course!
      case KMessageBox::Yes:
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
