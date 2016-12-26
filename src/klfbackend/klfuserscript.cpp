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
#include <QTextDocument> // Qt::escape()

#include <klfdefs.h>
#include <klfdebug.h>
#include <klfdatautil.h>
#include <klfpobj.h>

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
  Private() : KLFPropertizedObject("KLFUserScriptInfo")
  {
    refcount = 0;
    settings = NULL;
    scriptInfoError = KLFERR_NOERROR;

    registerBuiltInProperty(Category, QLatin1String("Category"));
    registerBuiltInProperty(Name, QLatin1String("Name"));
    registerBuiltInProperty(Author, QLatin1String("Author"));
    registerBuiltInProperty(Version, QLatin1String("Version"));
    registerBuiltInProperty(License, QLatin1String("License"));
    registerBuiltInProperty(KLFMinVersion, QLatin1String("KLFMinVersion"));
    registerBuiltInProperty(KLFMaxVersion, QLatin1String("KLFMaxVersion"));
    registerBuiltInProperty(SettingsFormUI, QLatin1String("SettingsFormUI"));
    registerBuiltInProperty(SpitsOut, QLatin1String("SpitsOut"));
    registerBuiltInProperty(SkipFormats, QLatin1String("SkipFormats"));
    registerBuiltInProperty(DisableInputs, QLatin1String("DisableInputs"));
    registerBuiltInProperty(InputFormUI, QLatin1String("InputFormUI"));
  }

  int refcount;
  inline int ref() { return ++refcount; }
  inline int deref() { return --refcount; }

  const KLFBackend::klfSettings *settings;

  QString fname;
  QString normalizedfname;
  QString sname;
  QString basename;
  int scriptInfoError;
  QString scriptInfoErrorString;

  QStringList notices;
  QStringList warnings;
  QStringList errors;


  void query_script_info(const QVariantMap& usconfig)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    scriptInfoError = 0;
    scriptInfoErrorString = QString();

    klfDbg("querying script information for script "<<fname) ;

    QByteArray scriptinfo;
    //    bool want_full_template = true;
    { // Query Script Info phase
      KLFBackendFilterProgram p(QObject::tr("User Script (ScriptInfo)"), settings, false, settings->tempdir);
      p.resErrCodes[KLFFP_NOSTART] = KLFERR_USERSCRIPT_NORUN;
      p.resErrCodes[KLFFP_NOEXIT] = KLFERR_USERSCRIPT_NONORMALEXIT;
      p.resErrCodes[KLFFP_NOSUCCESSEXIT] = KLFERR_PROGERR_USERSCRIPT;
      p.resErrCodes[KLFFP_NODATA] = KLFERR_USERSCRIPT_NOSCRIPTINFO;
      p.resErrCodes[KLFFP_DATAREADFAIL] = KLFERR_USERSCRIPT_NOSCRIPTINFO;

      QStringList usConfigEnvList = KLFUserScriptInfo::usConfigToEnvList(usconfig);
      klfDbg("Using config: "<< usConfigEnvList.join("; ")) ;
      p.addExecEnviron(usConfigEnvList);
      
      p.setArgv(QStringList() << fname << "--scriptinfo" << KLF_VERSION_STRING);
      
      bool ok = p.run(QString(), &scriptinfo);
      if (!ok) {
	scriptInfoError = p.resultStatus();
	scriptInfoErrorString = p.resultErrorString();
        klfWarning("Failed to query scriptinfo: "<<scriptInfoErrorString) ;
	return;
      }
    }
    scriptinfo.replace("\r\n", "\n");
    if (scriptinfo.isEmpty()) {
      scriptinfo = "ScriptInfo\n";
    }
    klfDbg("got scriptinfo="<<scriptinfo) ;
    // parse scriptinfo
    if (!scriptinfo.startsWith("ScriptInfo\n")) {
      scriptInfoError = KLFERR_USERSCRIPT_INVALIDSCRIPTINFO;
      scriptInfoErrorString = QObject::tr("User script did not provide valid --scriptinfo output.");
      qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo (missing header line).";
      return;
    }
    QList<QByteArray> lines = scriptinfo.split('\n');
    lines.removeAt(0); // skip of course the 'ScriptInfo\n' line !
    int k;
    for (k = 0; k < lines.size(); ++k) {
      QString line = QString::fromLocal8Bit(lines[k]);
      if (line.trimmed().isEmpty())
	continue;
      //  Key: Value   or
      //  Key[specifiers]: Value
      //    Key is [-A-Za-z0-9_.]+
      //    specifiers (optional) is [-A-Za-z0-9_.,]*, intendend to be a list of comma-separated options
      //    value is anything until end of line. Use 'base64' specifier if you need to have newlines in the value
      //        and encode the value as base64.
      QRegExp rx("([-A-Za-z0-9_.]+)(?:\\[([-A-Za-z0-9_.,]*)\\])?:\\s*(.*)");
      QRegExp boolrxtrue("(t(rue)?|y(es)?|on|1)");
      if (!rx.exactMatch(line)) {
	qWarning()<<KLF_FUNC_NAME<<": User script did not provide valid --scriptinfo.\nCannot parse line: "<<line;
	scriptInfoError = KLFERR_USERSCRIPT_INVALIDSCRIPTINFO;
	scriptInfoErrorString = QObject::tr("User script provided invalid --scriptinfo output.", "KLFBackend");
	return;
      }
      QString key = rx.cap(1);
      QString specstr = rx.cap(2);
      QStringList specs = specstr.split(',');
      QString val = rx.cap(3).trimmed();
      if (val.isEmpty())
	val = QString(); // empty value is null string
      if (specs.contains(QLatin1String("base64"))) {
	val = QString::fromLocal8Bit(QByteArray::fromBase64(val.toLatin1()));
      }
      klfDbg("key="<<key<<", value="<<val) ;
      if (key == QLatin1String("Notice")) {
	// emit a notice.
	notices << val;
	klfDbg("User script notice: "<<fname<< ": "<< val) ;
      } else if (key == QLatin1String("Warning")) {
	// emit a warning.
	warnings << val;
	klfWarning("User script warning: "<<fname<< ": "<< val) ;
      } else if (key == QLatin1String("Error")) {
	// emit an error.
	errors << val;
	klfWarning("User script error: "<<fname<< ": "<<val) ;
      }
      // now parse and set the property
      if (key == QLatin1String("Category")) {
	setProperty(Category, val);
	klfDbg("Read category: "<<property(Category)) ;
      } else if (key == QLatin1String("Name")) {
	setProperty(Name, val);
	klfDbg("Read name: "<<property(Name)) ;
      } else if (key == QLatin1String("Author") || key == QLatin1String("Authors")) {
	setProperty(Author, property(Author).toStringList() + (QStringList()<<val));
	klfDbg("Read (cumulated) author: "<<property(Author)) ;
      } else if (key == QLatin1String("Version")) {
	setProperty(Version, val);
	klfDbg("Read version: "<<property(Version)) ;
      } else if (key == QLatin1String("License")) {
	setProperty(License, val);
      } else if (key == QLatin1String("KLFMinVersion")) {
	setProperty(KLFMinVersion, val);
	klfDbg("Read klfMinVersion: "<<property(KLFMinVersion)) ;
      } else if (key == QLatin1String("KLFMaxVersion")) {
	setProperty(KLFMaxVersion, val);
	klfDbg("Read klfMaxVersion: "<<property(KLFMaxVersion)) ;
      } else if (key == QLatin1String("SettingsFormUI")) {
	if ( ! property(SettingsFormUI).toString().isEmpty() ) {
	  klfWarning("A user script ("<<sname<<") may not specify multiple settings UI forms.") ;
	}
	setProperty(SettingsFormUI, val);
	klfDbg("Read SettingsFormUI: "<<property(SettingsFormUI)) ;
      } else if (key == QLatin1String("SpitsOut")) {
	setProperty(SpitsOut, property(SpitsOut).toStringList() + val.split(QRegExp("(,|\\s+)")));
      } else if (key == QLatin1String("SkipFormats")) {
	setProperty(SkipFormats, property(SkipFormats).toStringList() + val.split(QRegExp("(,|\\s+)")));
      } else if (key == QLatin1String("DisableInputs")) {
	setProperty(DisableInputs, property(DisableInputs).toStringList() + val.split(QRegExp("(,|\\s+)")));
      } else if (key == QLatin1String("InputFormUI")) {
	if ( ! property(InputFormUI).toString().isEmpty() ) {
	  klfWarning("A user script ("<<sname<<") may not specify multiple input UI forms.") ;
	}
	setProperty(InputFormUI, val);
	klfDbg("Read InputFormUI: "<<property(InputFormUI)) ;
      }
      else {
	klfDbg("Custom userscript info key: "<<key<<", value="<<val);
	QVariant v = QVariant(val);
	// if the key is already present, morph the stored value into the first item of a variantlist and append
	// the new data.
	if (hasPropertyValue(key) && property(key).type() == QVariant::List) {
	  setProperty(key, QVariantList() << property(key).toList() << v);
	} else if (hasPropertyValue(key)) {
	  setProperty(key, QVariantList() << property(key) << v);
	} else {
	  setProperty(key, v);
	}
      }
    }
  }


  static QMap<QString,KLFRefPtr<Private> > userScriptInfoCache;
  
private:
  /* no copy constructor */
  Private(const Private& /*other*/) : KLFPropertizedObject("KLFUserScriptInfo") { }
};


// static
QMap<QString,KLFRefPtr<KLFUserScriptInfo::Private> > KLFUserScriptInfo::Private::userScriptInfoCache;

// static
KLFUserScriptInfo
KLFUserScriptInfo::forceReloadScriptInfo(const QString& scriptFileName, KLFBackend::klfSettings * settings,
                                         const QVariantMap& usconfig)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString normalizedfn = QFileInfo(scriptFileName).canonicalFilePath();
  Private::userScriptInfoCache.remove(normalizedfn);

  KLFUserScriptInfo usinfo(scriptFileName, settings, usconfig) ;
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
bool KLFUserScriptInfo::hasScriptInfoInCache(const QString& scriptFileName)
{
  QString normalizedfn = QFileInfo(scriptFileName).canonicalFilePath();
  return Private::userScriptInfoCache.contains(normalizedfn);
}

KLFUserScriptInfo::KLFUserScriptInfo(const QString& scriptFileName, const KLFBackend::klfSettings * settings,
                                     const QVariantMap& usconfig)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString normalizedfn = QFileInfo(scriptFileName).canonicalFilePath();
  if (Private::userScriptInfoCache.contains(normalizedfn)) {
    d = Private::userScriptInfoCache[normalizedfn];
  } else {
    d = new KLFUserScriptInfo::Private;

    d()->settings = settings;
    d()->fname = scriptFileName;
    d()->normalizedfname = normalizedfn;
    QFileInfo fi(scriptFileName);
    d()->sname = fi.fileName();
    d()->basename = fi.baseName();

    //KLF_ASSERT_NOT_NULL(settings, "Given NULL settings pointer! The KLFUserScript will not be initialized!", ; ) ;
    if (settings == NULL) {
      klfWarning("Given NULL settings pointer! The KLFUserScript will not be initialized!");
    } else {
      d()->query_script_info(usconfig);
      Private::userScriptInfoCache[normalizedfn] = d;
    }
  }
}

KLFUserScriptInfo::KLFUserScriptInfo(const KLFUserScriptInfo& copy)
  : KLFAbstractPropertizedObject()
{
  // will increase the refcount (thanks to KLFRefPtr)
  d = copy.d;
}

KLFUserScriptInfo::~KLFUserScriptInfo()
{
  d.setNull(); // will delete the data if refcount reaches zero (see KLFRefPtr)
}

QString KLFUserScriptInfo::fileName() const
{
  return d()->fname;
}
QString KLFUserScriptInfo::scriptName() const
{
  return d()->sname;
}
QString KLFUserScriptInfo::baseName() const
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


QString KLFUserScriptInfo::category() const { return info(Category).toString(); }
QString KLFUserScriptInfo::name() const { return info(Name).toString(); }
QString KLFUserScriptInfo::author() const { return info(Author).toStringList().join("; "); }
QStringList KLFUserScriptInfo::authorList() const { return info(Author).toStringList(); }
QString KLFUserScriptInfo::version() const { return info(Version).toString(); }
QString KLFUserScriptInfo::license() const { return info(License).toString(); }
QString KLFUserScriptInfo::klfMinVersion() const { return info(KLFMinVersion).toString(); }
QString KLFUserScriptInfo::klfMaxVersion() const { return info(KLFMaxVersion).toString(); }
QString KLFUserScriptInfo::settingsFormUI() const { return info(SettingsFormUI).toString(); }
QStringList KLFUserScriptInfo::spitsOut() const { return info(SpitsOut).toStringList(); }
QStringList KLFUserScriptInfo::skipFormats() const { return info(SkipFormats).toStringList(); }
QStringList KLFUserScriptInfo::disableInputs() const { return info(DisableInputs).toStringList(); }
QString KLFUserScriptInfo::inputFormUI() const { return info(InputFormUI).toString(); }


// KLF_DEFINE_PROPERTY_GET(KLFUserScriptInfo, QList<KLFUserScriptInfo::Param>, paramList) ;

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

  if (x == QLatin1String("Authors"))
    x = QLatin1String("Author");

  klfDbg("x="<<x) ;
  int id = d()->propertyIdForName(x);
  if (id < 0) {
    klfDbg("userscriptinfo does not have any information about "<<field<<" ("<<x<<")");
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

  // the name
  txt +=
    "<p style=\"-qt-block-indent: 0; text-indent: 0px; margin-top: 8px; margin-bottom: 0px\">\n"
    "<tt>" + QObject::tr("Script Name:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
    "<span style=\"font-weight:600;\">" + QFileInfo(fileName()).fileName().toHtmlEscaped() + "</span><br />\n";

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
  // the category
  txt += "<tt>" + QObject::tr("Category:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
    "<span style=\"font-weight:600;\">" + category().toHtmlEscaped() + "</span><br />\n";

  if (!license().isEmpty()) {
    // the license
    txt += "<tt>" + QObject::tr("License:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + license().toHtmlEscaped() + "</span><br />\n";
  }
  if (!spitsOut().isEmpty()) {
    // the output formats
    txt += "<tt>" + QObject::tr("Provides Formats:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + spitsOut().join(", ").toHtmlEscaped() + "</span><br />\n";
  }
  if (!skipFormats().isEmpty()) {
    // the skipped formats
    txt += "<tt>" + QObject::tr("Skipped Formats:", "[[user script info text]]") + "</tt>&nbsp;&nbsp;"
      "<span style=\"font-weight:600;\">" + skipFormats().join(", ").toHtmlEscaped() + "</span><br />\n";
  }

  /** \todo format extra non-standard properties */

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





// ----------------------------------------

struct KLFUserScriptFilterProcessPrivate
{
  KLF_PRIVATE_HEAD(KLFUserScriptFilterProcess)
  {
    usinfo = NULL;
  }

  KLFUserScriptInfo * usinfo;
};

KLFUserScriptFilterProcess::KLFUserScriptFilterProcess(const QString& scriptFileName,
						       const KLFBackend::klfSettings * settings)
  : KLFFilterProcess("User Script " + scriptFileName, settings)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg("scriptFileName= "<<scriptFileName) ;

  KLF_INIT_PRIVATE(KLFUserScriptFilterProcess) ;

  d->usinfo = new KLFUserScriptInfo(scriptFileName, settings);

  setArgv(QStringList() << d->usinfo->fileName());
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
