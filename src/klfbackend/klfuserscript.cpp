/***************************************************************************
 *   file klfuserscript.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

#include <QFileInfo>
#include <QDir>

#include <klfdefs.h>
#include <klfdebug.h>
#include <klfpobj.h>
#include <klfdatautil.h>

#include "klfbackend.h"
#include "klfbackend_p.h"
#include "klfuserscript.h"


/** \page pageUserScript User Scripts
 *
 * \todo ......... WRITE DOC................
 *
 * Format list (e.g. for 'spitsOut()' and 'skipFormats()':
 * \todo FORMAT LIST
 *
 * \bug NEEDS TESTING: user script output formats, skipped formats, parameters, etc.
 */





/*
static int read_spec_section(const QString& str, int fromindex, const QRegExp& seprx, QString * extractedPart)
{
  int i = fromindex;
  bool in_quote = false;

  QString s;

  while (i < str.length() && (in_quote || seprx.indexIn(str, i) != i)) {
    if (str[i] == '\\') {
      s.append(str[i]);
      if (i+1 < str.length())
	s.append(str[i+1]);
      i += 2; // skip next char, too. The actual escaping will be done with klfEscapedToData()
      continue;
    }
    if (str[i] == '"') {
      in_quote = !in_quote;
      ++i;
      continue;
    }
    s.append(str[i]);
    ++i;
  }

  *extractedPart = QString::fromLocal8Bit(klfEscapedToData(s.toLocal8Bit()));

  return i; // the position of the last char separator
}

*/





struct KLFUserScriptInfo::Private : public KLFPropertizedObject
{
  Private()
    : KLFPropertizedObject("KLFUserScriptInfo")
  {
    refcount = 0;
    scriptInfoError = KLFERR_NOERROR;

    registerBuiltInProperty(Category, QLatin1String("Category"));
    registerBuiltInProperty(Name, QLatin1String("Name"));
    registerBuiltInProperty(Author, QLatin1String("Author"));
    registerBuiltInProperty(Version, QLatin1String("Version"));
    registerBuiltInProperty(License, QLatin1String("License"));
    registerBuiltInProperty(KLFMinVersion, QLatin1String("KLFMinVersion"));
    registerBuiltInProperty(KLFMaxVersion, QLatin1String("KLFMaxVersion"));
    registerBuiltInProperty(SettingsFormUI, QLatin1String("SettingsFormUI"));
  }

  int refcount;
  inline int ref() { return ++refcount; }
  inline int deref() { return --refcount; }

  QString uspath;
  QString normalizedfname;
  QString sname;
  QString basename;
  int scriptInfoError;
  QString scriptInfoErrorString;

  QStringList notices;
  QStringList warnings;
  QStringList errors;


  void _set_xml_read_error(const QString& fullerrmsg)
  {
    scriptInfoError = 999;
    scriptInfoErrorString = fullerrmsg;
  }
  void _set_xml_parsing_error(const QString& xmlfname, const QString& errmsg)
  {
    scriptInfoError = 999;
    scriptInfoErrorString = QString("Error parsing scriptinfo XML contents: %1: %2")
      .arg(xmlfname).arg(errmsg);
  }

  void read_script_info()
  {
    scriptInfoError = 0;
    scriptInfoErrorString = QString();

    QString xmlfname = QDir::toNativeSeparators(uspath + "/scriptinfo.xml");
    QFile fxml(xmlfname);
    if ( ! fxml.open(QIODevice::ReadOnly) ) {
      _set_xml_read_error(QString("Can't open XML file %1: %2").arg(xmlfname).arg(fxml.errorString()));
      return;
    }

    QDomDocument doc("klfuserscript-info");
    QString errMsg; int errLine, errCol;
    bool r = doc.setContent(&fxml, false, &errMsg, &errLine, &errCol);
    if (!r) {
      _set_xml_read_error(QString("XML parse error: %1 (file %2 line %3 col %4)")
                          .arg(errMsg).arg(xmlfname).arg(errLine).arg(errCol));
      return;
    }
    fxml.close();

    QDomElement root = doc.documentElement();
    if (root.nodeName() != "klfuserscript-info") {
      _set_xml_parsing_error(xmlfname, QString("expected <klfuserscript-info> as root document element"));
      return;
    }
    
    // clear all properties
    setAllProperties(QMap<QString,QVariant>());

    // read XML contents
    QDomNode n;
    for (n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
      // try to convert the node to an element; ignore non-elements
      if ( n.nodeType() != QDomNode::ElementNode ) {
        continue;
      }
      QDomElement e = n.toElement();
      if ( e.isNull() ) {
        continue;
      }
      // parse the elements.
      QString val = e.text();
      if (val.isEmpty()) {
	val = QString(); // empty value is null string
      }
      if (e.nodeName() == "script") {
        if (!property(Script).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <script> element"));
          return;
        }
        setProperty(Script, val);
      } else if (e.nodeName() == "name") {
        if (!property(Name).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <name> element"));
          return;
        }
        setProperty(Name, val);
      } else if (e.nodeName() == "author") {
        setProperty(Author, property(Author).toStringList() + (QStringList()<<val));
      } else if (e.nodeName() == "version") {
        if (!property(Version).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <version> element"));
          return;
        }
        setProperty(Version, val);
      } else if (e.nodeName() == "license") {
        if (!property(License).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <license> element"));
          return;
        }
        setProperty(License, val);
      } else if (e.nodeName() == "klf-min-version") {
        if (!property(KLFMinVersion).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <klf-min-version> element"));
          return;
        }
        setProperty(KLFMinVersion, val);
      } else if (e.nodeName() == "klf-max-version") {
        if (!property(KLFMaxVersion).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <klf-max-version> element"));
          return;
        }
        setProperty(KLFMaxVersion, val);
      } else if (e.nodeName() == "settings-form-ui") {
        if (!property(SettingsFormUI).toString().isEmpty()) {
          _set_xml_parsing_error(xmlfname, QString("duplicate <settings-form-ui> element"));
          return;
        }
        setProperty(SettingsFormUI, val);
      } else if (e.nodeName() == property(Category).toString()) {
        // element node matching the category -- keep category-specific config as XML
        QByteArray xmlrepr;
        { QTextStream tstream(&xmlrepr);
          e.save(tstream, 2); }
        klfDbg("Read category-specific XML: " << xmlrepr);
        setProperty(CategorySpecificXmlConfig, xmlrepr);
      } else {
        _set_xml_parsing_error(xmlfname, QString("Unexpected element: %1").arg(e.nodeName()));
        return;
      }
    } // for all elements
  } // read_script_info()


  static QMap<QString,KLFRefPtr<Private> > userScriptInfoCache;
  
private:
  /* no copy constructor */
  Private(const Private& /*other*/) : KLFPropertizedObject("KLFUserScriptInfo") { }
};


// static
QMap<QString,KLFRefPtr<KLFUserScriptInfo::Private> > KLFUserScriptInfo::Private::userScriptInfoCache;

// static
KLFUserScriptInfo
KLFUserScriptInfo::forceReloadScriptInfo(const QString& userScriptFileName)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString normalizedfn = QFileInfo(userScriptFileName).canonicalFilePath();
  Private::userScriptInfoCache.remove(normalizedfn);

  KLFUserScriptInfo usinfo(userScriptFileName) ;
  if (usinfo.scriptInfoError() != KLFERR_NOERROR) {
    klfWarning(qPrintable(usinfo.scriptInfoErrorString()));
  }

  return usinfo;
}
// static
void KLFUserScriptInfo::clearCacheAll()
{
  // will decrease the refcounts if needed automatically (KLFRefPtr)
  Private::userScriptInfoCache.clear();
}


// static
bool KLFUserScriptInfo::hasScriptInfoInCache(const QString& userScriptFileName)
{
  QString normalizedfn = QFileInfo(userScriptFileName).canonicalFilePath();
  return Private::userScriptInfoCache.contains(normalizedfn);
}

KLFUserScriptInfo::KLFUserScriptInfo(const QString& userScriptFileName)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QFileInfo fi(userScriptFileName);
  QString normalizedfn = fi.canonicalFilePath();
  if (Private::userScriptInfoCache.contains(normalizedfn)) {
    d = Private::userScriptInfoCache[normalizedfn];
  } else {
    d = new KLFUserScriptInfo::Private;

    d()->uspath = userScriptFileName;
    d()->normalizedfname = normalizedfn;
    d()->sname = fi.fileName();
    d()->basename = fi.baseName();

    d()->read_script_info();
  }
}

KLFUserScriptInfo::KLFUserScriptInfo(const KLFUserScriptInfo& copy)
  : KLFAbstractPropertizedObject(copy)
{
  // will increase the refcount (thanks to KLFRefPtr)
  d = copy.d;
}

KLFUserScriptInfo::~KLFUserScriptInfo()
{
  d.setNull(); // will delete the data if refcount reaches zero (see KLFRefPtr)
}

QString KLFUserScriptInfo::userScriptPath() const
{
  return d()->uspath;
}
QString KLFUserScriptInfo::userScriptName() const
{
  return d()->sname;
}
QString KLFUserScriptInfo::userScriptBaseName() const
{
  return d()->basename;
}

int KLFUserScriptInfo::scriptInfoError() const
{
  return d()->scriptInfoError;
}
QString KLFUserScriptInfo::scriptInfoErrorString() const
{
  return d()->scriptInfoErrorString;
}

//protected
void KLFUserScriptInfo::setScriptInfoError(int code, const QString & msg)
{
  d()->scriptInfoError = code;
  d()->scriptInfoErrorString = msg;
}

QString KLFUserScriptInfo::script() const { return info(Script).toString(); }
QString KLFUserScriptInfo::category() const { return info(Category).toString(); }
QString KLFUserScriptInfo::name() const { return info(Name).toString(); }
QString KLFUserScriptInfo::author() const { return info(Author).toStringList().join("; "); }
QStringList KLFUserScriptInfo::authorList() const { return info(Author).toStringList(); }
QString KLFUserScriptInfo::version() const { return info(Version).toString(); }
QString KLFUserScriptInfo::license() const { return info(License).toString(); }
QString KLFUserScriptInfo::klfMinVersion() const { return info(KLFMinVersion).toString(); }
QString KLFUserScriptInfo::klfMaxVersion() const { return info(KLFMaxVersion).toString(); }
QString KLFUserScriptInfo::settingsFormUI() const { return info(SettingsFormUI).toString(); }

QByteArray KLFUserScriptInfo::categorySpecificXmlConfig() const
{ return info(CategorySpecificXmlConfig).toByteArray(); }


bool KLFUserScriptInfo::hasNotices() const
{
  return d->notices.size();
}
bool KLFUserScriptInfo::hasWarnings() const
{
  return d->warnings.size();
}
bool KLFUserScriptInfo::hasErrors() const
{
  return d->errors.size();
}

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QStringList, notices) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QStringList, warnings) ;

KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QStringList, errors) ;



QVariant KLFUserScriptInfo::info(int propId) const
{
  return d()->property(propId);
}

QVariant KLFUserScriptInfo::info(const QString& field) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QString x = field;

  if (x == QLatin1String("Authors")) {
    x = QLatin1String("Author");
  }

  klfDbg("x="<<x) ;
  int id = d()->propertyIdForName(x);
  if (id < 0) {
    klfDbg("KLFUserScriptInfo for "<<userScriptName()<<" does not have any information about "
           <<field<<" ("<<x<<")") ;
    return QVariant();
  }
  return info(id);
}

QStringList KLFUserScriptInfo::infosList() const
{
  return d()->propertyNameList();
}

QString KLFUserScriptInfo::objectKind() const { return d()->objectKind(); }


// protected. Used by eg. KLFExportTypeUserScriptInfo to normalize list property values.
void KLFUserScriptInfo::internalSetProperty(const QString& key, const QVariant &val)
{
  d()->setProperty(key, val);
}

const KLFPropertizedObject * KLFUserScriptInfo::pobj()
{
  return d();
}


static QString escapeListIntoTags(const QStringList& list, const QString& starttag, const QString& endtag)
{
  QString html;
  foreach (QString s, list) {
    html += starttag + s.toHtmlEscaped() + endtag;
  }
  return html;
}

QString KLFUserScriptInfo::htmlInfo(const QString& extra_css) const
{
  QString txt =
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
    "p, li { white-space: pre-wrap; }\n"
    "p.msgnotice { color: blue; font-weight: bold; margin: 2px 0px; }\n"
    "p.msgwarning { color: #a06000; font-weight: bold; margin: 2px 0px; }\n"
    "p.msgerror { color: #a00000; font-weight: bold; margin: 2px 0px; }\n"
    + extra_css + "\n"
    "</style></head>\n"
    "<body>\n";

  // any notices/warnings/errors go first
  if (hasNotices()) {
    txt += escapeListIntoTags(notices(), "<p class=\"msgnotice\">", "</p>\n");
  }
  if (hasWarnings()) {
    txt += escapeListIntoTags(warnings(), "<p class=\"msgwarning\">", "</p>\n");
  }
  if (hasErrors()) {
    txt += escapeListIntoTags(errors(), "<p class=\"msgerror\">", "</p>\n");
  }

  // the user script name (incl ".klfuserscript")
  txt +=
    "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 8px; margin-bottom: 0px\">\n"
    "<tt>" + QObject::tr("Script Name:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
    "<span style=\"font-weight:600;\">" + userScriptName().toHtmlEscaped() + "</span><br />\n";

  // the category
  txt += "<tt>" + QObject::tr("Category:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
    "<span style=\"font-weight:600;\">" + category().toHtmlEscaped() + "</span><br />\n";

  if (!version().isEmpty()) {
    // the version
    txt += "<tt>" + QObject::tr("Version:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + version().toHtmlEscaped() + "</span><br />\n";
  }
  if (!author().isEmpty()) {
    // the author
    txt += "<tt>" + QObject::tr("Author:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + author().toHtmlEscaped() + "</span><br />\n";
  }

  if (!license().isEmpty()) {
    // the license
    txt += "<tt>" + QObject::tr("License:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + license().toHtmlEscaped() + "</span><br />\n";
  }

  return txt;
}



// static
QMap<QString,QString> KLFUserScriptInfo::usConfigToStrMap(const QVariantMap& usconfig)
{
  QMap<QString,QString> mdata;
  for (QVariantMap::const_iterator it = usconfig.begin(); it != usconfig.end(); ++it)
    mdata[QLatin1String("KLF_USCONFIG_") + it.key()] = klfSaveVariantToText(it.value(), true);
  return mdata;
}

// static
QStringList KLFUserScriptInfo::usConfigToEnvList(const QVariantMap& usconfig)
{
  return klfMapToEnvironmentList(KLFUserScriptInfo::usConfigToStrMap(usconfig));
}





struct KLFBackendEngineUserScriptInfoPrivate : public KLFPropertizedObject
{
  KLF_PRIVATE_INHERIT_HEAD(KLFBackendEngineUserScriptInfo,
                           : KLFPropertizedObject("KLFBackendEngineUserScriptInfo"))
  {
    registerBuiltInProperty(KLFBackendEngineUserScriptInfo::SpitsOut, QLatin1String("SpitsOut"));
    registerBuiltInProperty(KLFBackendEngineUserScriptInfo::SkipFormats, QLatin1String("SkipFormats"));
    registerBuiltInProperty(KLFBackendEngineUserScriptInfo::DisableInputs, QLatin1String("DisableInputs"));
    registerBuiltInProperty(KLFBackendEngineUserScriptInfo::InputFormUI, QLatin1String("InputFormUI"));
  }

  void _set_xml_parsing_error(const QString& errmsg)
  {
    K->setScriptInfoError(1001, QString("Error parsing klf-backend-engine XML config: %1: %2")
                          .arg(K->userScriptBaseName()).arg(errmsg));
  }
  void parse_category_config(const QByteArray & ba)
  {
    QDomDocument doc("klf-backend-engine");
    QString errMsg; int errLine, errCol;
    bool r = doc.setContent(ba, false, &errMsg, &errLine, &errCol);
    if (!r) {
      K->setScriptInfoError(
          1001,
          QString("XML parse error: %1 (klf-backend-engine in %2, relative line %3 col %4)")
          .arg(errMsg).arg(K->userScriptBaseName()).arg(errLine).arg(errCol));
      return;
    }

    QDomElement root = doc.documentElement();
    if (root.nodeName() != "klf-backend-engine") {
      _set_xml_parsing_error(QString("expected <klf-backend-engine> element"));
      return;
    }
    
    // clear all properties
    setAllProperties(QMap<QString,QVariant>());

    // read XML contents
    QDomNode n;
    for (n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
      // try to convert the node to an element; ignore non-elements
      if ( n.nodeType() != QDomNode::ElementNode ) {
        continue;
      }
      QDomElement e = n.toElement();
      if ( e.isNull() ) {
        continue;
      }
      // parse the elements.
      QString val = e.text();
      if (val.isEmpty()) {
	val = QString(); // empty value is null string
      }
      if (e.nodeName() == "spits-out") {
        if (!property(KLFBackendEngineUserScriptInfo::SpitsOut).toString().isEmpty()) {
          _set_xml_parsing_error(QString("duplicate <spits-out> element"));
          return;
        }
        setProperty(KLFBackendEngineUserScriptInfo::SpitsOut, val);
      } else if (e.nodeName() == "disable-inputs") {
        if (!property(KLFBackendEngineUserScriptInfo::DisableInputs).toStringList().isEmpty()) {
          _set_xml_parsing_error(QString("duplicate <disable-inputs> element"));
          return;
        }
        QStringList lst;
        if (e.hasAttribute("selector")) {
          // all-except -> ALL_EXCEPT
          QString s = e.attribute("selector").toUpper();
          lst << s.replace('-', '_');
        }
        lst << val.split(' ', QString::SkipEmptyParts);
        setProperty(KLFBackendEngineUserScriptInfo::DisableInputs, lst);
      }
    }
  }
};



KLFBackendEngineUserScriptInfo::KLFBackendEngineUserScriptInfo(const QString& uspath)
  : KLFUserScriptInfo(uspath)
{
  KLF_INIT_PRIVATE(KLFBackendEngineUserScriptInfo) ;

  if (category() != "klf-backend-engine") {
    klfWarning("KLFBackendEngineUserScriptInfo instantiated for user script "
               << uspath << ", which is of category " << category()) ;
  } else {
    d->parse_category_config(categorySpecificXmlConfig());
  }
}

KLFBackendEngineUserScriptInfo::~KLFBackendEngineUserScriptInfo()
{
  KLF_DELETE_PRIVATE ;
}



QStringList KLFBackendEngineUserScriptInfo::spitsOut() const
{
  return info(SpitsOut).toStringList();
}
QStringList KLFBackendEngineUserScriptInfo::skipFormats() const
{
  return info(SkipFormats).toStringList();
}
QStringList KLFBackendEngineUserScriptInfo::disableInputs() const
{
  return info(DisableInputs).toStringList();
}
QString KLFBackendEngineUserScriptInfo::inputFormUI() const
{
  return info(InputFormUI).toString();
}











// ----------------------------------------

struct KLFUserScriptFilterProcessPrivate
{
  KLF_PRIVATE_HEAD(KLFUserScriptFilterProcess)
  {
    usinfo = NULL;
  }

  KLFUserScriptInfo * usinfo;
};

KLFUserScriptFilterProcess::KLFUserScriptFilterProcess(const QString& userScriptFileName,
						       const KLFBackend::klfSettings * settings)
  : KLFFilterProcess("User Script " + userScriptFileName, settings)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg("userScriptFileName= "<<userScriptFileName) ;

  KLF_INIT_PRIVATE(KLFUserScriptFilterProcess) ;

  d->usinfo = new KLFUserScriptInfo(userScriptFileName);

  setArgv(QStringList()
          << QDir::toNativeSeparators(d->usinfo->userScriptPath()+"/"+d->usinfo->script()) );
}


KLFUserScriptFilterProcess::~KLFUserScriptFilterProcess()
{
  delete d->usinfo;
  KLF_DELETE_PRIVATE ;
}

void KLFUserScriptFilterProcess::addUserScriptConfig(const QVariantMap& usconfig)
{
  QStringList envlist = KLFUserScriptInfo::usConfigToEnvList(usconfig);
  addExecEnviron(envlist);
}
