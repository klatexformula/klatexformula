/***************************************************************************
 *   file plugins/skin/skin.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#include <QtCore>
#include <QtGui>
#include <QtXml>

#include <klfmainwin.h>
#include <klfsettings.h>
#include <klflatexsymbols.h>
#include <klfconfig.h>
#include <klfguiutil.h>
#include <klfutil.h>

#include "skin.h"


SkinConfigWidget::SkinConfigWidget(QWidget *parent, KLFPluginConfigAccess *conf)
  : QWidget(parent), config(conf), _modified(false)
{
  setupUi(this);

  connect(cbxSkin, SIGNAL(activated(int)), this, SLOT(skinSelected(int)));
  connect(btnRefresh, SIGNAL(clicked()), this, SLOT(refreshSkin()));
  //  connect(btnInstallSkin, SIGNAL(clicked()), this, SLOT(installSkin()));
}


// static
Skin SkinConfigWidget::loadSkin(const QString& fn, bool getstylesheet)
{
  Skin skin;

  skin.fn = fn;

  QFile f(fn);
  if ( ! f.open(QIODevice::ReadOnly) ) {
    qWarning()<<KLF_FUNC_NAME<<": Can't read skin "<<fn<<"!";
    return skin;
  }

  QDomDocument doc("klf-skin");
  QString errMsg; int errLine, errCol;
  bool r = doc.setContent(&f, false, &errMsg, &errLine, &errCol);
  if (!r) {
    qWarning()<<KLF_FUNC_NAME<<": Error parsing file "<<fn<<": "<<errMsg<<" at line "<<errLine<<", col "<<errCol;
    return skin;
  }
  f.close();

  QDomElement root = doc.documentElement();
  if (root.nodeName() != "klf-skin") {
    qWarning("%s: Error parsing XML for skin `%s': Bad root node `%s'.\n",
	     KLF_FUNC_NAME, qPrintable(fn), qPrintable(root.nodeName()));
    return skin;
  }

  // read XML file
  QDomNode n;
  for (n = root.firstChild(); ! n.isNull(); n = n.nextSibling()) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( e.isNull() || n.nodeType() != QDomNode::ElementNode )
      continue;
    if ( e.nodeName() == "name" ) {
      skin.name = e.text();
      continue;
    } else if ( e.nodeName() == "author" ) {
      skin.author = e.text();
      continue;
    } else if ( e.nodeName() == "description" ) {
      skin.description = e.text();
      continue;
    } else if ( e.nodeName() == "stylesheet" ) {
      QString fnqss = e.text().trimmed();
      QFile fqss(fnqss);
      if (!fqss.exists() || !fqss.open(QIODevice::ReadOnly)) {
	qWarning()<<KLF_FUNC_NAME<<"Can't open qss-stylesheet file "<<fnqss<<" while reading skin "<<fn<<".";
	continue;
      }
      skin.stylesheet = QString::fromUtf8(fqss.readAll());
      continue;
    } else if ( e.nodeName() == "syntax-highlighting-scheme" ) {
      // read the syntax highlighting scheme...
      skin.overrideSHScheme = true;
      QDomNode nn;
      for (nn = e.firstChild(); ! nn.isNull(); nn = nn.nextSibling()) {
	QDomElement ee = nn.toElement();
	if (ee.isNull() || nn.nodeType() != QDomNode::ElementNode)
	  continue; // skip non-element nodes
	// read an item in the syntax highlighting scheme
	QTextCharFormat *which = NULL;
	if (ee.nodeName() == "keyword") { which = & skin.shscheme.fmtKeyword; }
	else if (ee.nodeName() == "comment") { which = & skin.shscheme.fmtComment; }
	else if (ee.nodeName() == "paren-match") { which = & skin.shscheme.fmtParenMatch; }
	else if (ee.nodeName() == "paren-mismatch") { which = & skin.shscheme.fmtParenMismatch; }
	else if (ee.nodeName() == "lonely-paren") { which = & skin.shscheme.fmtLonelyParen; }
	else {
	  qWarning()<<KLF_FUNC_NAME<<": Encountered unexpected tag "<<ee.nodeName()
		    <<" in element <syntax-highlighting-scheme> in file "<<fn;
	  continue;
	}
	// load the given format
	QTextFormat fmt = klfLoadVariantFromText(ee.text().toUtf8(), "QTextFormat").value<QTextFormat>();
	*which = fmt.toCharFormat();
      }
    } else {
      qWarning()<<KLF_FUNC_NAME<<": got unexpected tag "<<e.nodeName()<<" in <klf-skin> in file "<<fn<<".";
    }
  }

  klfDbg("read skin: "<<skin.name<<"; stylesheet="<<skin.stylesheet.simplified().mid(0, 50)
	 <<"[...]; keyword format is- fg:"<<skin.shscheme.fmtKeyword.foreground()
	 <<"/bg:"<<skin.shscheme.fmtKeyword.background()) ;

  // finally return the loaded skin
  return skin;
}



void SkinConfigWidget::loadSkinList(QString skinfn)
{
  qDebug("SkinConfigWidget::loadSkinList(%s)", qPrintable(skinfn));
  cbxSkin->clear();

  QStringList skindirs;
  skindirs
    << ":/plugindata/skin/skins/" << config->homeConfigPluginDataDir(true) + "/skins/";

  int k, j;
  int indf = -1;
  for (j = 0; j < skindirs.size(); ++j) {
    klfDbg("Checking dir "<<skindirs[j]) ;
    if ( ! QFile::exists(skindirs[j]) )
      if ( ! klfEnsureDir(skindirs[j]) )
	continue;

    QDir skindir(skindirs[j]);
    QStringList skinlist = skindir.entryList(QStringList() << "*.xml", QDir::Files);
    klfDbg("Skin list in dir "<<skindirs[j]<<": "<<skinlist) ;
    for (k = 0; k < skinlist.size(); ++k) {
      Skin skin = loadSkin(skindir.absoluteFilePath(skinlist[k]), false);
      QString skintitle = skin.name;
      QString fn = skindir.absoluteFilePath(skinlist[k]);
      qDebug("\tgot skin: %s : %s", qPrintable(skintitle), qPrintable(fn));
      if (fn == skinfn) {
	indf = cbxSkin->count();
      }
      cbxSkin->addItem(skintitle, QVariant::fromValue<Skin>(skin));
    }
  }
  // ---
  if (indf >= 0) {
    cbxSkin->blockSignals(true);
    cbxSkin->setCurrentIndex(indf);
    cbxSkin->blockSignals(false);
  }
  _modified = false;
  updateSkinDescription(cbxSkin->itemData(cbxSkin->currentIndex()).value<Skin>());
}

void SkinConfigWidget::skinSelected(int index)
{
  if (index < 0 || index >= cbxSkin->count())
    return;

  updateSkinDescription(cbxSkin->itemData(index).value<Skin>());
  
  _modified = true;
}

void SkinConfigWidget::refreshSkin()
{
  loadSkinList(currentSkinFn());
  _modified = true; // and re-set this skin after click on 'apply'
}

void SkinConfigWidget::updateSkinDescription(const Skin& skin)
{
  QString ds;
  ds =
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
    "<html><head>\n"
    "<meta name=\"qrichtext\" content=\"1\" />\n"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
    "<style type=\"text/css\">\n"
    "p, li {\n"
    "  white-space: pre-wrap;\n"
    "  -qt-block-indent:0;\n"
    "  text-indent:0px;\n"
    "}\n"
    "a {\n"
    "  text-decoration: underline;\n"
    "  color: #0000bf;\n"
    "}\n"
    "</style>\n"
    "</head><body>"
    "<pre style=\"font-weight: bold;\">"+tr("DESCRIPTION")+"</pre>\n";
  if (skin.description.length()) {
    ds += "<pre>"+skin.description+"</pre>\n";
  } else {
    ds += "<pre>"+tr("(No description provided)")+"</pre>\n";
  }
  ds += "<pre style=\"font-weight: bold;\">"+tr("AUTHOR")+"</pre>";
  if (skin.author.length()) {
    ds += "<pre>"+skin.author+"</pre>";
  } else {
    ds += "<pre>"+tr("(No author name provided)")+"</pre>\n";
  }
  if (skin.overrideSHScheme) {
    ds += "<pre><i>"+tr("This skin also provides a syntax highlighting color scheme")+"</i></pre>";
  } else {
    ds += "<pre><i>"+tr("This skin will not alter syntax highlighting color scheme")+"</i></pre>";
  }
  ds += "</body></html>";

  lblSkinDescription->setText(ds);
}



/* SKINS SHOULD BE INSTALLED VIA EXTENSIONS (Qt RCC FILES)
 * -------------------------------------------------------

void SkinConfigWidget::installSkin()
{
  QString docs = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
  QString fname =
    QFileDialog::getOpenFileName(this, tr("Open Skin File"), docs,
				 tr("Qt Style Sheet Files (*.qss);;All files (*)"));
  if (fname.isEmpty())
    return;
  if ( ! QFile::exists(fname) ) {
    qWarning()<<"SkinConfigWidget::installSkin: File "<<fname<<" does not exist.";
    return;
  }
  QString target = config->homeConfigPluginDataDir(true) + "/stylesheets/" + QFileInfo(fname).fileName();
  if ( QFile::exists(target) ) {
    QMessageBox::StandardButton res =
      QMessageBox::warning(this, tr("Skin Already Exists") ,
			   tr("A Skin Named \"%1\" is already installed. Overwrite it?")
			   .arg(QFileInfo(fname).fileName()) ,
			   QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
    if (res != QMessageBox::Yes)
      return;
    // yes, overwrite
    QFile::remove(target);
  }
  bool ok = QFile::copy(fname, target);
  if ( !ok ) {
    QMessageBox::critical(this, tr("Error"),
			  tr("Failed to install skin \"%1\".").arg(QFileInfo(fname).fileName()));
  }
  refreshSkin();
}
*/



// --------------------------------------------------------------------------------


void SkinPlugin::initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *rwconfig)
{
  qDebug("Initializing Skin plugin (compiled for KLF version %s)", KLF_VERSION_STRING);
  Q_INIT_RESOURCE(skindata);

  _mainwin = mainWin;
  _app = app;
  //  _defaultstyle = NULL;
  _defaultstyle = app->style();
  // aggressively take possession of this style object
  _defaultstyle->setParent(this);

  _config = rwconfig;

  { // add links for skins to what's new page
    QString s =
      tr("<p>Some new <b>interface skins</b> are available in this version. You may want "
	 "to try the <a href=\"%1\">papyrus skin</a>, the <a href=\"%2\">galaxy skin</a>, "
	 "the <a href=\"%3\">flat skin</a>, or fall back to the regular "
	 "<a href=\"%4\">default skin</a>.</p>",
	 "[[help new features additional text]]")
      .arg("klfaction:/skin_set?skin=:/plugindata/skin/skins/papyrus.xml",
	   "klfaction:/skin_set?skin=:/plugindata/skin/skins/galaxy.xml",
	   "klfaction:/skin_set?skin=:/plugindata/skin/skins/flat.xml",
	   "klfaction:/skin_set?skin=:/plugindata/skin/skins/default.xml");
    _mainwin->addWhatsNewText(s);
    _mainwin->registerHelpLinkAction("/skin_set", this, "changeSkinHelpLinkAction", true);
  }

  // ensure reasonable non-empty value in config
  if ( rwconfig->readValue("skinfilename").isNull() ||
       !QFile::exists(rwconfig->readValue("skinfilename").toString()) )
    rwconfig->writeValue("skinfilename", QString(":/plugindata/skin/skins/default.xml"));

  rwconfig->makeDefaultValue("noSyntaxHighlightingChange", QVariant(false));

  applySkin(rwconfig, true);
}

void SkinPlugin::changeSkin(const QString& newSkin, bool force)
{
  qDebug()<<KLF_FUNC_NAME<<": skin="<<newSkin;
  if (!force && _config->readValue("skinfilename").toString() == newSkin) {
    klfDbg("newSkin="<<newSkin<<" is same as already-set skin "<<_config->readValue("skinfilename").toString());
    return; // already right
  }

  _config->writeValue("skinfilename", QVariant::fromValue<QString>(newSkin));
  Skin skin = applySkin(_config, false);

  // apply syntax highlighting scheme only when skin is CHANGED (thus here and not in applySkin())

  if ( ! _config->readValue("noSyntaxHighlightingChange").toBool()  &&  skin.overrideSHScheme) {
    klfDbg("Applying syntax highlighting scheme, keyword.isValid()="<<skin.shscheme.fmtKeyword.isValid()) ;
    if (skin.shscheme.fmtKeyword.isValid())
      klfconfig.SyntaxHighlighter.fmtKeyword = skin.shscheme.fmtKeyword;
    if (skin.shscheme.fmtComment.isValid())
      klfconfig.SyntaxHighlighter.fmtComment = skin.shscheme.fmtComment;
    if (skin.shscheme.fmtParenMatch.isValid())
      klfconfig.SyntaxHighlighter.fmtParenMatch = skin.shscheme.fmtParenMatch;
    if (skin.shscheme.fmtParenMismatch.isValid())
      klfconfig.SyntaxHighlighter.fmtParenMismatch = skin.shscheme.fmtParenMismatch;
    if (skin.shscheme.fmtLonelyParen.isValid())
      klfconfig.SyntaxHighlighter.fmtLonelyParen = skin.shscheme.fmtLonelyParen;
  }

  emit skinChanged(newSkin);
}
void SkinPlugin::changeSkinHelpLinkAction(const QUrl& link)
{
  qDebug()<<KLF_FUNC_NAME<<": link="<<link;
  changeSkin(link.queryItemValue("skin"));
}


Skin SkinPlugin::applySkin(KLFPluginConfigAccess *config, bool /*isStartUp*/)
{
  klfDbg("Applying skin!");
  QString ssfn = config->readValue("skinfilename").toString();
  Skin skin =  SkinConfigWidget::loadSkin(ssfn);
  QString stylesheet = skin.stylesheet;

  klfDbg("Applyin skin "<<skin.name<<" (from file "<<ssfn<<")") ;

  KLFPleaseWaitPopup pleaseWaitPopup(tr("Applying skin <i>%1</i>, please wait ...").arg(skin.name), _mainwin);

  if (_mainwin->isVisible())
    pleaseWaitPopup.showPleaseWait();

  if (_app->style() != _defaultstyle) {
    _app->setStyle(_defaultstyle);
    // but aggressively keep possession of style
    _defaultstyle->setParent(this);
    // and refresh mainwin's idea of application's style
    _mainwin->setProperty("widgetStyle", QVariant(QString::null));
  }
  // set style sheet to whole application (doesn't work...)
  //  _app->setStyleSheet(stylesheet);

  // set top-level widgets' klfTopLevelWidget property to TRUE, and
  // apply our style sheet to all top-level widgets
  QWidgetList toplevelwidgets = _app->topLevelWidgets();
  int k;
  for (k = 0; k < toplevelwidgets.size(); ++k) {
    QWidget *w = toplevelwidgets[k];
    QString objnm = w->objectName();
    if (!_baseStyleSheets.contains(objnm))
      _baseStyleSheets[objnm] = w->styleSheet();

    w->setProperty("klfTopLevelWidget", QVariant(true));
    w->setAttribute(Qt::WA_StyledBackground);
    w->setStyleSheet(_baseStyleSheets[objnm] + "\n" + stylesheet);
  }
  // #ifndef Q_WS_WIN
  //   // BUG? tab widget in settings dialog is always un-skinned after an "apply"...?
  //   KLFSettings *settingsDialog = _mainwin->findChild<KLFSettings*>();
  //   if (settingsDialog) {
  //     QTabWidget * tabs = settingsDialog->findChild<QTabWidget*>("tabs");
  //     if (tabs) {
  //       qDebug("Setting stylesheet to tabs");
  //       tabs->setStyleSheet(stylesheet);
  //     }
  //   }
  // #endif

  return skin;
}

QWidget * SkinPlugin::createConfigWidget(QWidget *parent)
{
  QWidget *w = new SkinConfigWidget(parent, _config);
  w->setProperty("SkinConfigWidget", true);
  connect(this, SIGNAL(skinChanged(const QString&)), w, SLOT(loadSkinList(const QString&)));
  return w;
}

void SkinPlugin::loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }
  SkinConfigWidget * o = qobject_cast<SkinConfigWidget*>(confwidget);
  o->loadSkinList(config->readValue("skinfilename").toString());
  o->chkNoSyntaxHighlightingChange->setChecked(config->readValue("noSyntaxHighlightingChange").toBool());
  // reset modified status
  o->getModifiedAndReset();
}
void SkinPlugin::saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config)
{
  klfDbg("called.");
  if (confwidget->property("SkinConfigWidget").toBool() != true) {
    fprintf(stderr, "Error: bad config widget given !!\n");
    return;
  }

  SkinConfigWidget * o = qobject_cast<SkinConfigWidget*>(confwidget);

  QString skinfn = o->currentSkinFn();

  config->writeValue("skinfilename", QVariant::fromValue<QString>(skinfn));
  config->writeValue("noSyntaxHighlightingChange", o->chkNoSyntaxHighlightingChange->isChecked());

  if ( o->getModifiedAndReset() ) {
    klfDbg("skin setting modified. setting new skin");
    // "true"=force skin change, because we just wrote the config, and it will
    // believe there was no skin change
    changeSkin(skinfn, true);
  }
}





// Export Plugin
Q_EXPORT_PLUGIN2(skin, SkinPlugin);


