/***************************************************************************
 *   file klfkateplugin.cpp
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

#include <QLabel>
#include <QGridLayout>
#include <QMouseEvent>

#include <ktexteditor/document.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>

#include "klfkateplugin.h"
#include "klfkateplugin_config.h"


KLFKtePlugin *KLFKtePlugin::plugin = NULL;

K_PLUGIN_FACTORY_DEFINITION(KLFKtePluginFactory,
			    registerPlugin<KLFKtePlugin>("ktexteditor_klf");
			    registerPlugin<KLFKteConfig>("ktexteditor_klf_config");
			    )
  ;
K_EXPORT_PLUGIN(KLFKtePluginFactory("ktexteditor_klf", "ktexteditor_plugins"))
  ;

KLFKtePlugin::KLFKtePlugin(QObject *parent, const QVariantList &/*args*/)
  : KTextEditor::Plugin(parent)
{
  plugin = this;
  readConfig();
}

KLFKtePlugin::~KLFKtePlugin()
{
  plugin = NULL;
}

void KLFKtePlugin::addView(KTextEditor::View *view)
{
  KLFKtePluginView *nview = new KLFKtePluginView(view);
  pViews.append(nview);
}

void KLFKtePlugin::removeView(KTextEditor::View *view)
{
  for (int z = 0; z < pViews.size(); z++) {
    if (pViews.at(z)->parentClient() == view) {
      KLFKtePluginView *nview = pViews.at(z);
      pViews.removeAll(nview);
      delete nview;
    }
  }
}

void KLFKtePlugin::readConfig()
{
  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  KLFKteConfigData::inst()->readConfig(&cg);
}

void KLFKtePlugin::writeConfig()
{
  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  KLFKteConfigData::inst()->writeConfig(&cg);
}



// ----------------------------------


KLFKteLatexRunThread::KLFKteLatexRunThread(QObject *parent)
  : QThread(parent),   _hasnewinfo(false), _abort(false)
{
}

KLFKteLatexRunThread::~KLFKteLatexRunThread()
{
  _mutex.lock();
  _abort = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
  wait();
}

void KLFKteLatexRunThread::run()
{
  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  KLFBackend::klfOutput output;
  QImage img;

  for (;;) {
    if (_abort)
      return;

    // fetch info
    _mutex.lock();
    input = _input;
    settings = _settings;
    settings.epstopdfexec = "";
    _hasnewinfo = false;
    _mutex.unlock();

    // render equation
    if ( input.latex.isEmpty() ) {
      emit previewError(i18n("No Latex Equation"), -1000);
    } else {
      // and GO!
      fprintf(stderr, "KLFKteLatexRunThread::run(): running getLatexFormula ...\n");
      output = KLFBackend::getLatexFormula(input, settings);
      img = output.result;
      fprintf(stderr, "... status=%d\n", output.status);
      if (output.status == 0) {
	if (img.width() > 600 || img.height() > 200)
	  img = img.scaled(QSize(600, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      } else {
	img = QImage();
      }
    }

    _mutex.lock();
    bool abort = _abort;
    bool hasnewinfo = _hasnewinfo;
    _mutex.unlock();

    if (abort)
      return;
    if (hasnewinfo)
      continue;

    // otherwise we have a ready preview!

    if (!img.isNull())
      emit previewAvailable(img);
    else
      emit previewError(output.errorstr, output.status);

    _mutex.lock();
    _condnewinfoavail.wait(&_mutex);
    _mutex.unlock();
  }
}
bool KLFKteLatexRunThread::setInput(const KLFBackend::klfInput& input)
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
void KLFKteLatexRunThread::setSettings(const KLFBackend::klfSettings& settings)
{
  _mutex.lock();
  _settings = settings;
  _hasnewinfo = true;
  _condnewinfoavail.wakeOne();
  _mutex.unlock();
}




// --------------------------------


KLFKtePreviewWidget::KLFKtePreviewWidget(KTextEditor::View *vparent)
  : QWidget(vparent, Qt::Window|Qt::FramelessWindowHint|Qt::CustomizeWindowHint|
	    Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint)
{
  setAttribute(Qt::WA_ShowWithoutActivating, true);

  QGridLayout *l = new QGridLayout(this);
  lbl = new QLabel(this);
  lbl->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  QLabel *klfLinks = new QLabel(i18n("<a href=\"klfkteaction:/invoke_klf\">open in KLatexFormula</a>&nbsp;|&nbsp;"
				     "<a href=\"klfkteaction:/no_autopopup\">don't popup automatically</a>&nbsp;|&nbsp;"
				     "<a href=\"klfkteaction:/close\">close</a>"), this);
  
  l->addWidget(lbl, 0, 0, 2, 2, Qt::AlignCenter);
  l->addWidget(klfLinks, 2, 0, 2, 1);
  l->setColumnStretch(0, 1);

  installEventFilter(this);
  lbl->installEventFilter(this);
  vparent->installEventFilter(this);

  connect(klfLinks, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)));
}
KLFKtePreviewWidget::~KLFKtePreviewWidget()
{
}

bool KLFKtePreviewWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == parentWidget() && event->type() == QEvent::FocusOut) {
    hide();
  }
  if (event->type() == QEvent::MouseButtonPress) {
    hide();
  }
  return QWidget::eventFilter(obj, event);
}

void KLFKtePreviewWidget::linkActivated(const QString& url)
{
  if (url == "klfkteaction:/invoke_klf") {
    emit invokeKLF();
  } else if (url == "klfkteaction:/close") {
    hide();
  } else if (url == "klfkteaction:/no_autopopup") {
    KLFKteConfigData::inst()->autopopup = false;
    KLFKtePlugin::self()->writeConfig();
    hide();
  }
}


void KLFKtePreviewWidget::showPreview(const QImage& preview, const QPoint& globalpos)
{
  qDebug()<<KLF_FUNC_NAME<<"()";
  lbl->setPixmap(QPixmap::fromImage(preview));
  move(globalpos+QPoint(0,30));
  show();
  adjustSize();
  resize(sizeHint()+QSize(4,4));

  setWindowOpacity(1.0 - (KLFKteConfigData::inst()->transparencyPercent / 100.0));
}



// ---------------------------------


KLFKtePluginView::KLFKtePluginView(KTextEditor::View *view)
  : QObject(view),
    KXMLGUIClient(view),
    pView(view)
{
  setComponentData(KLFKtePluginFactory::componentData());

  KLFBackend::detectSettings(&klfsettings);
  
  KAction *aPreviewSel = new KAction(i18n("Preview Selection or Current Equation"), this);
  aPreviewSel->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
  KAction *aInvokeKLF = new KAction(i18n("Invoke KLatexFormula"), this);
  actionCollection()->addAction("klf_preview_selection", aPreviewSel);
  actionCollection()->addAction("klf_invoke_klf", aInvokeKLF);
  connect(aPreviewSel, SIGNAL(triggered()), this, SLOT(slotPreview()));
  connect(aInvokeKLF, SIGNAL(triggered()), this, SLOT(slotInvokeKLF()));
  
  setXMLFile("klfkatepluginui.rc");

  connect(pView->document(), SIGNAL(highlightingModeChanged(KTextEditor::Document*)),
	  this, SLOT(slotHighlightingModeChanged(KTextEditor::Document *)));
  slotHighlightingModeChanged(pView->document()); // initialize pIsGoodHighlightingMode

  connect(pView->document(), SIGNAL(textChanged(KTextEditor::Document*)),
	  this, SLOT(slotReparseCurrentContext()));
  connect(pView, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)),
	  this, SLOT(slotReparseCurrentContext()));
  connect(pView, SIGNAL(selectionChanged(KTextEditor::View *)),
	  this, SLOT(slotSelectionChanged()));

  pPreview = new KLFKtePreviewWidget(pView);

  pLatexRunThread = new KLFKteLatexRunThread(this);
  pLatexRunThread->start();
  pLatexRunThread->setSettings(klfsettings);

  connect(pLatexRunThread, SIGNAL(previewAvailable(const QImage&)),
	  this, SLOT(slotReadyPreview(const QImage&)));
  connect(pLatexRunThread, SIGNAL(previewError(const QString&, int)),
	  this, SLOT(slotHidePreview()));

  connect(pPreview, SIGNAL(invokeKLF()), this, SLOT(slotInvokeKLF()));
}

KLFKtePluginView::~KLFKtePluginView()
{
  delete pPreview;
}

void KLFKtePluginView::slotHighlightingModeChanged(KTextEditor::Document *document)
{
  if (document == pView->document()) {
    if (KLFKteConfigData::inst()->onlyLatexMode)
      pIsGoodHighlightingMode =
	! QString::compare(pView->document()->highlightingMode(), "LaTeX", Qt::CaseInsensitive);
    else
      pIsGoodHighlightingMode = true;
  }
}

void KLFKtePluginView::slotReparseCurrentContext()
{
  qDebug()<<KLF_FUNC_NAME<<"()";

  if (!pIsGoodHighlightingMode)
    return;

  KTextEditor::Document *doc = pView->document();

  KTextEditor::Cursor curPos = pView->cursorPosition();

  if (pView->selectionRange().isValid()) {
    slotSelectionChanged();
    return;
  }

  // math-mode regex
  //  QRegExp rxmm("[^\\](\\\\(begin\\{(equation|eqnarray)\\*?\\}|\\(|\\[)|\\$|\\$\\$)"
  //   	       "(.*[^\\])" // 4th capture
  //   	       "(\\\\(end\\{(equation|eqnarray)\\*?\\}|\\)|\\])|\\$|\\$\\$)");
  //  QRegExp rxmm("(\\\\(\\(|\\[)|\\$|(\\$\\$))"
  //	       "(.*)" // 4th capture
  //	       "(\\\\(\\)|\\])|\\$|\\$\\$)");
  QRegExp rxmm("(\\\\(\\(|\\[|begin\\{(equation|eqnarray)\\*?\\})|\\$|\\$\\$)"
  	       "(.*)" // 4th capture
  	       "(\\\\(\\)|\\]|end\\{(equation|eqnarray)\\*?\\})|\\$|\\$\\$)");
  rxmm.setMinimal(true);

  QString text;
  const int K = 20; // search within beginning of document and K lines after current position
  int l = 0;
  int curOffset = curPos.column();
  while (l < doc->lines() && l <= curPos.line() + K) {
    QString line = doc->line(l)+"\n";
    text += line;
    if (l < curPos.line()) // as long as we are above the current cursor, count the offset to cursor
      curOffset += line.length();
    ++l;
  }

  qDebug()<<KLF_FUNC_NAME<<": collected full text is:\n"<<text;
  qDebug()<<KLF_FUNC_NAME<<": regex pattern is:\n"<<rxmm.pattern();
  qDebug()<<KLF_FUNC_NAME<<": Matching regex to doc text. ; cur cursor position=(line="<<curPos.line()
	  <<",col="<<curPos.column()<<"), curOffset="<<curOffset<<": "<<("..."+text.mid(curOffset-10,10)+"\033[7m"+text.mid(curOffset,1)+"\033[0m"+text.mid(curOffset+1,10)+"...");
  // now match regex to text.
  int matchIndex = rxmm.lastIndexIn(text, curOffset-1);
  qDebug()<<KLF_FUNC_NAME<<": matchIndex="<<matchIndex<<": "<<("..."+text.mid(matchIndex-10,10)+"\033[7m"+text.mid(matchIndex,1)+"\033[0m"+text.mid(matchIndex+1,10)+"...");
  // check if
  //  * we matched at all
  //  * the cursor is in the equation (ie. matchpos+matchlength >= cursorpos)
  if (matchIndex < 0 || matchIndex < curOffset-rxmm.matchedLength()) {
    qDebug()<<KLF_FUNC_NAME<<": match failed. matchIndex="<<matchIndex;
    pCurMathContext.isValidMathContext = false;
    slotHidePreview();
    return;
  }
  qDebug()<<KLF_FUNC_NAME<<": match succeeded! match pos="<<matchIndex;

  // match, setup the math context structure
  pCurMathContext.isValidMathContext = true;
  pCurMathContext.latexequation = rxmm.cap(4);
  QString mathmodebegin = rxmm.cap(1);
  QString mathmodeend = rxmm.cap(5);
  pCurMathContext.mathmodebegin = mathmodebegin;
  pCurMathContext.mathmodeend = mathmodeend;
  QString beginenviron = rxmm.cap(3);
  if (beginenviron.length() > 0) {
    mathmodebegin = "\\begin{"+beginenviron+"*}"; // force '*' for no line numbering
    mathmodeend = "\\end{"+beginenviron+"*}"; // force '*' for no line numbering
  }
  pCurMathContext.klfmathmode = mathmodebegin + "..." + mathmodeend;

  if (KLFKteConfigData::inst()->autopopup)
    slotPreview();
}

void KLFKtePluginView::slotSelectionChanged()
{
  slotHidePreview();

  if (!pIsGoodHighlightingMode)
    return;

  pCurMathContext.isValidMathContext = true; // ....... hack .........
  pCurMathContext.latexequation = pView->selectionText();
  pCurMathContext.mathmodebegin = "\\[";
  pCurMathContext.mathmodeend = "\\]";
  pCurMathContext.klfmathmode = "\\[ ... \\]";
}

void KLFKtePluginView::slotPreview()
{
  slotPreview(pCurMathContext);
}

void KLFKtePluginView::slotPreview(const MathContext& context)
{
  if (!pIsGoodHighlightingMode)
    return;

  // NOTE: Don't complain if the equation doesn't parse, my latex parser can very easily be fooled if eg.
  // the cursor is placed between to inlined equations like : ...and we define $R$ and $S$ to...
  // if the cursor is on the "and", then it sees the (wrong) inlined equation "$ and $"

  KLFBackend::klfInput klfinput;
  klfinput.latex = context.latexequation;
  klfinput.mathmode = context.klfmathmode;
  klfinput.preamble = KLFKteConfigData::inst()->preamble;
  klfinput.fg_color = qRgb(0, 0, 0); // black
  klfinput.bg_color = qRgba(255, 255, 255, 0); // transparent
  klfinput.dpi = 180;

  pLatexRunThread->setInput(klfinput);
}

void KLFKtePluginView::slotHidePreview()
{
  pPreview->hide();
}

void KLFKtePluginView::slotReadyPreview(const QImage& preview)
{
  if (!pIsGoodHighlightingMode)
    return;

  qDebug()<<KLF_FUNC_NAME<<"()";
  pPreview->showPreview(preview, pView->cursorPositionCoordinates()+pView->mapToGlobal(pView->pos()));
}


void KLFKtePluginView::slotInvokeKLF()
{
  if (pCurMathContext.isValidMathContext) {
    KProcess::startDetached(QStringList()
			    << KLFKteConfigData::inst()->klfpath
			    << "-I"
			    << "--latexinput="+pCurMathContext.latexequation
			    << "--mathmode="+pCurMathContext.klfmathmode
			    );
  } else {
    KProcess::startDetached(QStringList()
			    << KLFKteConfigData::inst()->klfpath
			    );
  }
}





#include "klfkateplugin.moc"
