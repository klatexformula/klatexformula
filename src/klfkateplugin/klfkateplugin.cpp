/***************************************************************************
 *   file klfkateplugin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QPainter>
#include <QTimer>
#include <QMessageBox>

#include <ktexteditor/document.h>
#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/attribute.h>

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

#include "ui_klfktepreviewwidget.h"


KLFKtePlugin *KLFKtePlugin::staticInstance = NULL;

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
  staticInstance = this;

  pConfigData = new KLFKteConfigData();

  pLatexPreviewThread = new KLFLatexPreviewThread(this);
  pLatexPreviewThread->start();
  pLatexPreviewThread->setPriority(QThread::LowestPriority);

  readConfig();
}

KLFKtePlugin::~KLFKtePlugin()
{
  staticInstance = NULL;

  delete pConfigData;

  // note: destruction of pConfigData and pLatexPreviewThread is dealt with automatically
  // via QObject destruction chain, because we're their parent.
}

void KLFKtePlugin::addView(KTextEditor::View *view)
{
  KLFKtePluginView *nview = new KLFKtePluginView(this, view);
  pViews.append(nview);

  connect(nview, SIGNAL(requestedDontPopupAutomatically()),
          this, SLOT(setDontPopupAutomatically()));
}

void KLFKtePlugin::removeView(KTextEditor::View *view)
{
  for (int z = 0; z < pViews.size(); z++) {
    if (pViews.at(z)->parentClient() == view) {
      KLFKtePluginView *nview = pViews.at(z);
      pViews.removeAll(nview);
      delete nview;
      break;
    }
  }
}

void KLFKtePlugin::readConfig()
{
  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  pConfigData->readConfig(&cg);
}

void KLFKtePlugin::writeConfig()
{
  KConfigGroup cg(KGlobal::config(), "KLatexFormula Plugin");
  pConfigData->writeConfig(&cg);
}


void KLFKtePlugin::setDontPopupAutomatically()
{
  pConfigData->autopopup = false;
  writeConfig();
}



// --------------------------------


KLFKtePixmapWidget::KLFKtePixmapWidget(QWidget *parent)
  : QWidget(parent), pPix(QPixmap()), pSemiTransparent(false)
{
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  setMinimumSize(QSize(10,10));
  setFocusPolicy(Qt::NoFocus);
}
KLFKtePixmapWidget::~KLFKtePixmapWidget()
{
}

void KLFKtePixmapWidget::setSemiTransparent(bool val)
{
  pSemiTransparent = val;
}

void KLFKtePixmapWidget::setPix(const QPixmap& pix)
{
  pPix = pix;
  setMinimumSize(pPix.size());
  //resize(pPix.size());
  update();
}

void KLFKtePixmapWidget::paintEvent(QPaintEvent */*e*/)
{
  int x = (width()-pPix.width()) / 2;
  int y = (height()-pPix.height()) / 2;
  QPainter p(this);
  p.save();
  if (pSemiTransparent) {
    p.setOpacity(0.5);
  }
  p.drawPixmap(x,y,pPix);
  p.restore();
}


// --------------------------------


KLFKtePreviewWidget::KLFKtePreviewWidget(KLFKtePluginView * pluginView, KTextEditor::View *vparent)
  : QWidget(vparent, Qt::ToolTip), pPluginView(pluginView)
{
  u = new Ui::KLFKtePreviewWidget();
  u->setupUi(this);

  setAttribute(Qt::WA_ShowWithoutActivating, true);
  //setAttribute(Qt::WA_PaintOnScreen, true);

  // smaller font
  QFont f = u->klfLinks->font();
  f.setPointSize(QFontInfo(f).pointSize()-1);
  u->klfLinks->setFont(f);

  installEventFilter(this);
  u->lbl->installEventFilter(this);
  vparent->installEventFilter(this);

  connect(u->klfLinks, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)));
}
KLFKtePreviewWidget::~KLFKtePreviewWidget()
{
  delete u;
}

bool KLFKtePreviewWidget::eventFilter(QObject *obj, QEvent *event)
{
  //  qDebug()<<"KLFKte: Object: "<<obj<<", Event: "<<event->type();
  if (obj == parentWidget() && (event->type() == QEvent::FocusOut ||
				event->type() == QEvent::WindowDeactivate ||
				event->type() == QEvent::WindowStateChange)) {
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
    hide();
    emit requestedDontPopupAutomatically();
  }
}

void KLFKtePreviewWidget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);
}
static int popupXPos(int mywidth, int viewx, int viewwidth, int viewposx)
{
  if (mywidth > viewwidth) {
    // center the popup on the view, but not past zero on the left
    return qMax(0, viewx - (mywidth-viewwidth)/2);
  }
  // rel. to view: at viewposx=0, show at x=0, at viewposx=viewwidth, show at x=viewwidth-mywidth
  return  viewx   +    (viewwidth-mywidth) * viewposx/viewwidth ;
  /*
  int maxleftshift = qMin(400, globalposx*2/3);
  // if mywidth <= 100 -> show right under cursor
  if (mywidth <= 100)
    return 0;
  if (mywidth >= 600) {
    return -maxleftshift;
  }
  // interpolate between 0 (at mywidth=100) and -maxleftshift (at mywidth=600)
  return 0  +  (-maxleftshift) * (mywidth-100)/(600-100) ; */
}
void KLFKtePreviewWidget::showPreview(const QImage& preview, QWidget *view, const QPoint& pos)
{
  u->lblErrorNotice->hide();
  u->lbl->setSemiTransparent(false);

  QPoint globViewPos = view->mapToGlobal(view->pos());
  u->lbl->setPix(QPixmap::fromImage(preview));
  //adjustSize();
  u->klfLinks->setShown(pPluginView->configData()->popupLinks);
  resize(sizeHint()+QSize(4,4));
  move(popupXPos(width(), globViewPos.x(), view->width(), pos.x()),
       globViewPos.y()+pos.y()+35);

  show();

  if (pPluginView->configData()->transparencyPercent)
    setWindowOpacity(1.0 - (pPluginView->configData()->transparencyPercent / 100.0));
  
  // schedule re-paint to workaround bug where label is not repainted some times
  QTimer::singleShot(20, u->lbl, SLOT(repaint()));
}

void KLFKtePreviewWidget::showPreviewError(const QString& errstr)
{
  Q_UNUSED(errstr) ;

  u->lblErrorNotice->show();
  u->lbl->setSemiTransparent(true);
}


// ---------------------------------




KLFKtePluginView::KLFKtePluginView(KLFKtePlugin * mainPlugin, KTextEditor::View *view)
  : QObject(view),
    KXMLGUIClient(view),
    pMainPlugin(mainPlugin),
    pView(view),
    pIsGoodHighlightingMode(false),
    pParser(NULL),
    pPreventNextShow(false)
{
  setComponentData(KLFKtePluginFactory::componentData());

  KLFBackend::detectSettings(&klfsettings);

  klfsettings.outlineFonts = false; // we don't care about font outlining.
  klfsettings.wantPDF = false; // and about PDF.
  klfsettings.wantSVG = false; // and also about SVG.
  
  klfWarning("DEBUG: new view!") ;

  aPreviewSel = new KAction(i18n("Show Popup For Current Equation"), this);
  aInvokeKLF = new KAction(i18n("Invoke KLatexFormula"), this);
  aInvokeKLF->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
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
  	  this, SLOT(slotReparseCurrentContext()));

  connect(pView, SIGNAL(contextMenuAboutToShow(KTextEditor::View*, QMenu*)),
	  this, SLOT(slotContextMenuAboutToShow(KTextEditor::View*, QMenu*)));

  pPreview = new KLFKtePreviewWidget(this, pView);

  pContLatexPreview = new KLFContLatexPreview(pMainPlugin->latexPreviewThread());
  pContLatexPreview->setPreviewSize(configData()->popupMaxSize);
  klfWarning("preview size is "<<pContLatexPreview->previewSize());
  pContLatexPreview->setSettings(klfsettings);

  connect(pContLatexPreview, SIGNAL(previewImageAvailable(const QImage&)),
	  this, SLOT(slotReadyPreview(const QImage&)), Qt::QueuedConnection);
  connect(pContLatexPreview, SIGNAL(previewError(const QString&, int)),
  	  this, SLOT(slotPreviewError(const QString&, int)), Qt::QueuedConnection);


  connect(pPreview, SIGNAL(invokeKLF()), this, SLOT(slotInvokeKLF()));
  connect(pPreview, SIGNAL(requestedDontPopupAutomatically()),
          this, SIGNAL(requestedDontPopupAutomatically()));


  klfDbg("pView()->document()'s class name: "<<pView->document()->metaObject()->className()) ;
  if (pView->document()->inherits("KateDocument")) {
    // OK, we got a KatePart editor.
    KTextEditor::HighlightInterface *hiface = qobject_cast<KTextEditor::HighlightInterface*>(pView->document());
    pParser = createKatePartParser(pView->document(), hiface);
  } else {
    pParser = createDummyParser(pView->document());
  }
}

KLFKtePluginView::~KLFKtePluginView()
{
  if (pParser != NULL) {
    delete pParser;
  }
  if (pPreview != NULL) {
    delete pPreview;
  }
}

void KLFKtePluginView::slotHighlightingModeChanged(KTextEditor::Document *document)
{
  if (document == pView->document()) {
    pIsGoodHighlightingMode =
      ! QString::compare(pView->document()->highlightingMode(), "LaTeX", Qt::CaseInsensitive);
  }
}

void KLFKtePluginView::slotReparseCurrentContext()
{
  if (!pIsGoodHighlightingMode)
    return;

  KLF_ASSERT_NOT_NULL(pParser, "parser object is NULL!", return; ) ;

  Cur curPos = pView->cursorPosition();

  if (!curPos.isValid())
    return;

  MathModeContext context = pParser->parseContext(curPos);

  if (context == pCurMathContext) {
    if (configData()->autopopup)
      slotSamePreview();
    return;
  }

  pCurMathContext = context;

  klfDbg("parsed math mode context : "<<pCurMathContext) ;

  if (!pCurMathContext.isValid()) {
    slotHidePreview();
    klfDbg("Not in math mode.") ;
    return;
  }

  if (configData()->autopopup)
    slotPreview();

  return;

  /*
  KTextEditor::Document *doc = pView->document();

  // ------------------

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

  //  qDebug()<<KLF_FUNC_NAME<<": collected full text is:\n"<<text;
  //  qDebug()<<KLF_FUNC_NAME<<": regex pattern is:\n"<<rxmm.pattern();
  //  qDebug()<<KLF_FUNC_NAME<<": Matching regex to doc text. ; cur cursor position=(line="<<curPos.line()
  //	  <<",col="<<curPos.column()<<"), curOffset="<<curOffset<<": "<<("..."+text.mid(curOffset-10,10)+"\033[7m"+text.mid(curOffset,1)+"\033[0m"+text.mid(curOffset+1,10)+"...");
  // now match regex to text.
  int matchIndex = rxmm.lastIndexIn(text, curOffset-1);
  //  qDebug()<<KLF_FUNC_NAME<<": matchIndex="<<matchIndex<<": "<<("..."+text.mid(matchIndex-10,10)+"\033[7m"+text.mid(matchIndex,1)+"\033[0m"+text.mid(matchIndex+1,10)+"...");
  // check if
  //  * we matched at all
  //  * the cursor is in the equation (ie. matchpos+matchlength >= cursorpos)
  if (matchIndex < 0 || matchIndex < curOffset-rxmm.matchedLength()) {
    //    qDebug()<<KLF_FUNC_NAME<<": match failed. matchIndex="<<matchIndex;
    pCurMathContext.isValidMathContext = false;
    slotHidePreview();
    return;
  }
  //  qDebug()<<KLF_FUNC_NAME<<": match succeeded! match pos="<<matchIndex;

  // match, setup the math context structure
  pCurMathContext.isValidMathContext = true;
  pCurMathContext.latexequation = rxmm.cap(4);
  QString mathmodebegin = rxmm.cap(1);
  QString mathmodeend = rxmm.cap(5);
  pCurMathContext.mathmodebegin = mathmodebegin;
  pCurMathContext.mathmodeend = mathmodeend;
  QString beginenviron = rxmm.cap(3);
  if (beginenviron.length() > 0) {
    if (beginenviron == "equation") {
      mathmodebegin = "\\["; // latex doesn't like 'equation*' environment when opened in KLF (?)
      mathmodeend = "\\]";
    } else {
      mathmodebegin = "\\begin{"+beginenviron+"*}"; // force '*' for no line numbering
      mathmodeend = "\\end{"+beginenviron+"*}"; // force '*' for no line numbering
    }
  }
  pCurMathContext.klfmathmode = mathmodebegin + "..." + mathmodeend;

  if (KLFKteConfigData::inst()->autopopup)
    slotPreview();
  */
}






void KLFKtePluginView::slotSelectionChanged()
{
  slotHidePreview();

  if (!pIsGoodHighlightingMode)
    return;

  pCurMathContext = MathModeContext();
  KTextEditor::Range selrange = pView->selectionRange();
  pCurMathContext.start = selrange.start();
  pCurMathContext.end = selrange.end();
  pCurMathContext.latexmath = pView->selectionText();
  pCurMathContext.startcmd = "\\[";
  pCurMathContext.endcmd = "\\]";
}

void KLFKtePluginView::slotContextMenuAboutToShow(KTextEditor::View */*view*/, QMenu * /*menu*/)
{
  pPreventNextShow = true; // don't show the preview

  // Actually not needed, this is done via XML gui
  /*
    QMenu *klftoolsmenu = menu->addMenu(tr("KLatexFormula Tools"));
    klftoolsmenu->addAction(aPreviewSel);
    klftoolsmenu->addAction(aInvokeKLF);
  */
}


void KLFKtePluginView::slotPreview()
{
  if (pView->selectionRange().isValid()) {
    slotSelectionChanged(); // make the selection the current math mode context
  }

  slotPreview(pCurMathContext);
}

void KLFKtePluginView::slotSamePreview()
{
  if (pCurMathContext.isValid()) {
    //    pContLatexPreview->reemitPreviewAvailable();
    /// \bug ....................
    ///   ### What was the bug?
    slotReadyPreview(pLastPreview);
  } else {
    slotHidePreview();
  }
}

void KLFKtePluginView::slotPreview(const MathModeContext& context)
{
  if (!pIsGoodHighlightingMode)
    return;

  KLFBackend::klfInput klfinput;
  klfinput.latex = context.latexmath;
  klfinput.mathmode = context.fullMathModeWithoutNumbering();
  klfinput.preamble = configData()->preamble;
  klfinput.fg_color = qRgb(0, 0, 0); // black
  klfinput.bg_color = qRgba(255, 255, 255, 0); // transparent
  klfinput.dpi = 180;

  pContLatexPreview->setInput(klfinput);
}

void KLFKtePluginView::slotHidePreview()
{
  pPreview->hide();
}

void KLFKtePluginView::slotReadyPreview(const QImage& preview)
{
  klfWarning("preview!! size="<<preview.size()) ;

  if (!pIsGoodHighlightingMode)
    return;

  if (pPreventNextShow) {
    pPreventNextShow = false; // reset this flag
    return;
  }

  pLastPreview = preview;

  pPreview->showPreview(preview, pView, pView->cursorPositionCoordinates());
}

void KLFKtePluginView::slotPreviewError(const QString& errorString, int errorCode)
{
  Q_UNUSED(errorCode) ;

  pPreview->showPreviewError(errorString);
}


void KLFKtePluginView::slotInvokeKLF()
{
  if (pCurMathContext.isValid()) {
    // given that we use startDetached(), --daemonize is superfluous
    KProcess::startDetached(QStringList()
			    << configData()->klfpath
			    << "-I"
			    << "--latexinput="+pCurMathContext.latexmath.trimmed()
			    << "--mathmode="+pCurMathContext.fullMathModeWithoutNumbering()
			    );
  } else {
    KProcess::startDetached(QStringList()
			    << configData()->klfpath
			    );
  }
}





#include "klfkateplugin.moc"
