/***************************************************************************
 *   file klfmainwin.cpp
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

#include <qcombobox.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qimage.h>
#include <qpushbutton.h>
#include <qtextbrowser.h>
#include <qfile.h>
#include <qfont.h>
#include <qclipboard.h>
#include <qdragobject.h>

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

#include <klfbackend.h>

#include "klfdata.h"
#include "klfmainwinui.h"
#include "klfhistorybrowser.h"
#include "klfsettings.h"
#include "klfmainwin.h"



#define min_i(x,y) ( (x)<(y) ? (x) : (y) )




// KLatexFormula version
extern const int version_maj, version_min;



/* This is for backwards compatibility with older versions than 2.0.0
 * These classes and functions are only used in loadHistory() and the loaded stuff is directly
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

QDataStream& operator<<(QDataStream& str, const KLFOldHistContext& c)
{
  return str << (Q_INT8)c.changecolors << c.fgcolor << c.bgcolor << (Q_INT8)c.bgtransparent << (Q_INT32)c.imgborderoffset << (Q_INT32)c.imgdpi
	     << (Q_INT8)c.latexmathmode << c.latexmathmodedelimiters << c.latexpreamble;

}
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

QDataStream& operator<<(QDataStream& str, const KLFOldHistoryElement& item)
{
  return str << item.id << item.tm << item.preview << item.latexstring << item.context;
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





KLFMainWin::KLFMainWin()
 : KMainWindow()
{
  mMainWidget = new KLFMainWinUI(this);

  KConfig *cfg = kapp->config();

  cfg->setGroup("BackendSettings");

  _settings.klfclspath = locate("appdata", "klatexformula.cls");
  _settings.klfclspath = _settings.klfclspath.left(_settings.klfclspath.findRev('/'));

  _settings.tempdir = cfg->readEntry("tempdir", KGlobal::instance()->dirs()->findResourceDir("tmp", "/"));
  _settings.latexexec = cfg->readEntry("latexexec", KStandardDirs::findExe("latex"));
  _settings.dvipsexec = cfg->readEntry("dvipsexec", KStandardDirs::findExe("dvips"));
  _settings.gsexec = cfg->readEntry("gsexec", KStandardDirs::findExe("gs"));
  _settings.epstopdfexec = cfg->readEntry("gsexec", KStandardDirs::findExe("epstopdf"));

  _output.status = 0;
  _output.errorstr = QString();
  _output.result = QPixmap();
  _output.pngdata = QByteArray();
  _output.epsdata = QByteArray();
  _output.pdfdata = QByteArray();



  mHistoryBrowser = new KLFHistoryBrowser(&_history, 0); // no parent, to prevent it from hiding us.


  // load styles
  loadStyles();

  // load history
  loadHistory();

  // setup mMainWidget UI correctly

  mMainWidget->btnClear->setPixmap(locate("appdata", "pics/clear.png"));
  //  mMainWidget->btnClear->setFixedSize(mMainWidget->btnClear->pixmap()->size() + QSize(4, 4));
  mMainWidget->btnExpand->setPixmap(locate("appdata", "pics/switchexpanded.png"));
  //  mMainWidget->btnClear->setFixedSize(mMainWidget->btnExpand->pixmap()->size() + QSize(4, 4));
  mMainWidget->btnEvaluate->setIconSet(QPixmap(locate("appdata", "pics/evaluate.png")));
  mMainWidget->btnEvaluate->setText(i18n("Evaluate"));
  mMainWidget->btnHistory->setPixmap(locate("appdata", "pics/history.png"));

  QFont font = mMainWidget->btnDrag->font();
  font.setPointSize(font.pointSize()-2);
  mMainWidget->btnDrag->setFont(font);
  mMainWidget->btnCopy->setFont(font);
  mMainWidget->btnSave->setFont(font);

  font = mMainWidget->txePreamble->font();
  font.setPointSize(font.pointSize()-2);
  mMainWidget->txePreamble->setFont(font);

  mMainWidget->frmOutput->setEnabled(false);

  _shrinkedsize = mMainWidget->frmMain->sizeHint() + QSize(3, 3);
  _expandedsize.setWidth(mMainWidget->frmMain->sizeHint().width() + mMainWidget->frmDetails->sizeHint().width() + 3);
  _expandedsize.setHeight(mMainWidget->frmMain->sizeHint().height() + 3);


  mMainWidget->frmDetails->hide();

  setCentralWidget(mMainWidget);

  mMainWidget->lblOutput->installEventFilter(this);
  mMainWidget->txeLatex->installEventFilter(this);

  setFixedSize(_shrinkedsize);


  connect(mMainWidget->btnClear, SIGNAL(clicked()), this, SLOT(slotClear()));
  connect(mMainWidget->btnEvaluate, SIGNAL(clicked()), this, SLOT(slotEvaluate()));
  connect(mMainWidget->btnHistory, SIGNAL(toggled(bool)), this, SLOT(slotHistory(bool)));
  connect(mMainWidget->btnExpand, SIGNAL(clicked()), this, SLOT(slotExpandOrShrink()));
  connect(mMainWidget->btnCopy, SIGNAL(clicked()), this, SLOT(slotCopy()));
  connect(mMainWidget->btnDrag, SIGNAL(released()), this, SLOT(slotDrag()));
  connect(mMainWidget->btnSave, SIGNAL(clicked()), this, SLOT(slotSave()));
  connect(mMainWidget->btnSettings, SIGNAL(clicked()), this, SLOT(slotSettings()));
  connect(mMainWidget->btnQuit, SIGNAL(clicked()), this, SLOT(close()));
  connect(mStyleMenu, SIGNAL(activated(int)), this, SLOT(slotLoadStyle(int)));
  connect(mMainWidget->btnSaveStyle, SIGNAL(clicked()), this, SLOT(slotSaveStyle()));

  connect(mHistoryBrowser, SIGNAL(restoreFromHistory(KLFData::KLFHistoryItem, bool)),
	  this, SLOT(restoreFromHistory(KLFData::KLFHistoryItem, bool)));
  connect(mHistoryBrowser, SIGNAL(refreshHistoryBrowserShownState(bool)),
	  this, SLOT(slotHistoryButtonRefreshState(bool)));
}


KLFMainWin::~KLFMainWin()
{
  saveStyles();
  saveHistory();

  if (mStyleMenu)
    delete mStyleMenu;
  if (mHistoryBrowser)
    delete mHistoryBrowser; // we aren't its parent
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
    KLFData::KLFStyle s1 = { i18n("default"), qRgb(0, 0, 0), qRgba(255, 255, 255, 0), "\\[ ... \\]", "", 1200 };
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

void KLFMainWin::loadHistory()
{
  _history = KLFData::KLFHistoryList();

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

	stream >> KLFData::KLFHistoryItem::MaxId >> _history;

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

	  KLFData::KLFHistoryItem::MaxId = KLFOldHistoryElement::max_id+1; // just to be sure

	  for (uint i = 0; i < old_hist.size(); ++i) {
	    KLFData::KLFHistoryItem h;
	    h.id = old_hist[i].id;
	    h.datetime = old_hist[i].tm;
	    h.latex = old_hist[i].latexstring;
	    h.preview = old_hist[i].preview;
	    h.style.name = i18n("[old style import]");
	    h.style.fg_color = old_hist[i].context.changecolors ? old_hist[i].context.fgcolor.rgb() : qRgb(0, 0, 0);
	    h.style.bg_color = old_hist[i].context.changecolors ?
	      ( (old_hist[i].context.bgtransparent) ? qRgba(255, 255, 255, 0) : old_hist[i].context.bgcolor.rgb() )
	      : qRgba(255, 255, 255, 255); // remember: by default, in old versions BG was opaque
	    h.style.mathmode = old_hist[i].context.latexmathmode ? old_hist[i].context.latexmathmodedelimiters : "...";
	    h.style.preamble = old_hist[i].context.latexpreamble;
	    h.style.dpi = old_hist[i].context.imgdpi;
	    // old_hist[i].context.imgborderoffset is dropped because the concept is obsolete

	    // 	    printf("DEBUG: Appended item id=%d, latex='%s', previewsize=%dx%d, style/mathmode='%s'\n"
	    // 		   "\tfrom item latexmathmode=%d, latexmathmodedelimiters='%s'\n", h.id, h.latex.ascii(), h.preview.width(), h.preview.height(),
	    // 		   h.style.mathmode.ascii(), (int)old_hist[i].context.latexmathmode, old_hist[i].context.latexmathmodedelimiters.ascii());

	    _history.append(h);
	  }

	}
      } // yes to import
    } // QFile::exists(oldhistoryfile)
  } // else-block of QFile::exists(newhistoryfile)

  mHistoryBrowser->historyChanged();
}

void KLFMainWin::saveHistory()
{
  QString s = locateLocal("appdata", "history");
  if (s.isEmpty()) {
    KMessageBox::error(this, i18n("Error: Unable to make local file for saving history!\n"), i18n("Error"));
    return;
  }
  QFile f(s);
  if ( ! f.open(IO_WriteOnly | IO_Raw) ) {
    KMessageBox::error(this, i18n("Error: Unable to write to history file!\n"), i18n("Error"));
    return;
  }
  QDataStream stream(&f);
  stream << QString("KLATEXFORMULA_HISTORY") << (Q_INT16)version_maj << (Q_INT16)version_min
	 << KLFData::KLFHistoryItem::MaxId << _history;
}

void KLFMainWin::restoreFromHistory(KLFData::KLFHistoryItem h, bool restorestyle)
{
  mMainWidget->txeLatex->setText(h.latex);
  if (restorestyle) {
    mMainWidget->kccFg->setColor(QColor(qRed(h.style.fg_color), qGreen(h.style.fg_color), qBlue(h.style.fg_color)));
    mMainWidget->kccBg->setColor(QColor(qRed(h.style.bg_color), qGreen(h.style.bg_color), qBlue(h.style.bg_color)));
    mMainWidget->chkBgTransparent->setChecked(qAlpha(h.style.bg_color) == 0);
    mMainWidget->chkMathMode->setChecked(h.style.mathmode.simplifyWhiteSpace() != "...");
    if (h.style.mathmode.simplifyWhiteSpace() != "...")
      mMainWidget->cbxMathMode->setCurrentText(h.style.mathmode);
    mMainWidget->txePreamble->setText(h.style.preamble);
    mMainWidget->spnDPI->setValue(h.style.dpi);
  }

  mMainWidget->lblOutput->setPixmap(h.preview);
  mMainWidget->frmOutput->setEnabled(false);
}

void KLFMainWin::slotHistoryButtonRefreshState(bool on)
{
  mMainWidget->btnHistory->setOn(on);
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
    KMessageBox::error(this, i18n("Error"), i18n("Error: %1").arg(_output.errorstr));
    mMainWidget->lblOutput->setText("");
    mMainWidget->frmOutput->setEnabled(false);
  }
  if (_output.status > 0) {
    KLFProgErr::showError(this, _output.errorstr);
    mMainWidget->frmOutput->setEnabled(false);
  }
  if (_output.status == 0) {
    // ALL OK

    QPixmap sc = _output.result.convertToImage().smoothScale(QSize(250, 50), QImage::ScaleMin);
    mMainWidget->lblOutput->setPixmap(sc);

    mMainWidget->frmOutput->setEnabled(true);

    KLFData::KLFHistoryItem hi = { KLFData::KLFHistoryItem::MaxId++, QDateTime::currentDateTime(), input.latex, sc, KLFData::KLFStyle() };
    hi.style = currentStyle();

    mHistoryBrowser->addToHistory(hi);
  }

  mMainWidget->btnEvaluate->setEnabled(true); // re-enable our button

}

void KLFMainWin::slotClear()
{
  mMainWidget->txeLatex->setText("");
}

void KLFMainWin::slotHistory(bool showhist)
{
  mHistoryBrowser->setShown(showhist);
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
  QImage res = _output.result.convertToImage();
  QImageDrag *drag = new QImageDrag(res, this);
  //  drag->setImage(res);
  QPixmap p;
  p = res.smoothScale(QSize(200, 100), QImage::ScaleMin);
  drag->setPixmap(p, QPoint(20, 10));
  drag->dragCopy();
}
void KLFMainWin::slotCopy()
{
  kapp->clipboard()->setImage(_output.result.convertToImage(), QClipboard::Clipboard);
}
void KLFMainWin::slotSave()
{
  QString filterformat = "";
  QStringList formats = QImage::outputFormatList();

  for (QStringList::iterator it = formats.begin(); it != formats.end(); ++it) {
    if (!filterformat.isEmpty()) filterformat += "\n";
    filterformat += "*."+(*it).lower()+"|"+(*it).lower()+" Image";
  }

  filterformat = "*.eps|EPS PostScript\n"+filterformat;
  filterformat = "*.pdf|PDF Portable Document Format\n"+filterformat;

  filterformat = "*.jpg|Standard JPEG Image\n"+filterformat;
  filterformat = "*.png|Standard PNG Image\n"+filterformat;
  
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
    fsav.open(IO_WriteOnly | IO_Raw);
    //    for (int k = 0; k < dataptr->size(); ++k) fsav.putch(dataptr->at(k));
    fsav.writeBlock(*dataptr);
  } else {
    _output.result.save(fname, format.upper());
  }

}


KLFData::KLFStyle KLFMainWin::currentStyle() const
{
  KLFData::KLFStyle sty;

  sty.name = i18n("[ no name ]");
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

  /*  sty.name = name;
      sty.fg_color = mMainWidget->kccFg->color().rgb();
      QColor bgc = mMainWidget->kccBg->color();
      sty.bg_color = qRgba(bgc.red(), bgc.green(), bgc.blue(), mMainWidget->chkBgTransparent->isChecked() ? 0 : 255 );
      sty.mathmode = (mMainWidget->chkMathMode->isChecked() ? mMainWidget->cbxMathMode->currentText() : "...");
      sty.preamble = mMainWidget->txePreamble->text();
      sty.dpi = mMainWidget->spnDPI->value(); */
  sty = currentStyle();
  sty.name = name;

  _styles.append(sty);
  refreshStylePopupMenus();
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
  KMainWindow::saveProperties(cfg);
}
void KLFMainWin::readProperties(KConfig *cfg)
{
  KMainWindow::readProperties(cfg);
}





#include "klfmainwin.moc"