/***************************************************************************
 *   file klfexporter.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2016 by Philippe Faist
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


#include <QVariant>
#include <QVariantList>

#include <klfdefs.h>

#include <klfdatautil.h>

#include <klfexporter.h>
#include <klfexporter_p.h>


struct KLFExporterPrivate
{
  KLF_PRIVATE_HEAD(KLFExporter)
  {
  }

  QString errorString;
};


KLFExporter::KLFExporter()
{
  KLF_INIT_PRIVATE( KLFExporter ) ;
}

KLFExporter::~KLFExporter()
{
  KLF_DELETE_PRIVATE ;
}


QStringList KLFExporter::fileNameExtensionsFor(const QString & )
{
  return QStringList();
}


QString KLFExporter::errorString() const
{
  return d->errorString;
}

void KLFExporter::clearErrorString()
{
  d->errorString = QString();
}
void KLFExporter::setErrorString(const QString & errorString)
{
  d->errorString = errorString;
}


// =============================================================================

struct KLFExporterManagerPrivate
{
  KLF_PRIVATE_HEAD( KLFExporterManager )
  {
  }

  QList<KLFExporter*> pExporters;
};

KLFExporterManager::KLFExporterManager()
{
  KLF_INIT_PRIVATE( KLFExporterManager ) ;
}
KLFExporterManager::~KLFExporterManager()
{
  foreach (KLFExporter * exporter, d->pExporters) {
    if (exporter != NULL) {
      delete exporter;
    }
  }

  KLF_DELETE_PRIVATE ;
}

void KLFExporterManager::registerExporter(KLFExporter *exporter)
{
  KLF_ASSERT_NOT_NULL( exporter, "Refusing to register NULL exporter object!",  return ) ;

  d->pExporters.append(exporter);

#ifdef KLF_DEBUG
  klfDbg("Exporter list is now :");
  foreach (KLFExporter * exporter, d->pExporters) {
    klfDbg("        - " << qPrintable(exporter->exporterName())) ;
  }
#endif
}
void KLFExporterManager::unregisterExporter(KLFExporter *exporter)
{
  d->pExporters.removeAll(exporter);
}


KLFExporter* KLFExporterManager::exporterByName(const QString & exporterName)
{
  int k;
  for (k = 0; k < d->pExporters.size(); ++k) {
    if (d->pExporters[k]->exporterName() == exporterName) {
      return d->pExporters[k];
    }
  }
  return NULL;
}

QList<KLFExporter*> KLFExporterManager::exporterList()
{
  return d->pExporters;
}




// =============================================================================
// =============================================================================


// static members for specific exporters


// static
QMap<QString,QMap<qint64,QString> > KLFTempFileUriExporter::tempFilesCache
   = QMap<QString,QMap<qint64,QString> >();





// =============================================================================
// =============================================================================
// =============================================================================

// user script exporter definitions


// KLFExportTypeUserScriptInfo stuff


struct KLFExportTypeUserScriptInfoPrivate : public KLFPropertizedObject
{
  KLF_PRIVATE_INHERIT_HEAD(KLFExportTypeUserScriptInfo,
                           : KLFPropertizedObject("KLFExportTypeUserScriptInfo"))
  {
    registerBuiltInProperty(KLFExportTypeUserScriptInfo::Formats, QLatin1String("Formats"));
    registerBuiltInProperty(KLFExportTypeUserScriptInfo::InputDataType, QLatin1String("InputDataType"));
    registerBuiltInProperty(KLFExportTypeUserScriptInfo::HasStdoutOutput, QLatin1String("HasStdoutOutput"));
    registerBuiltInProperty(KLFExportTypeUserScriptInfo::MimeExportProfilesXmlFile,
                            QLatin1String("MimeExportProfilesXmlFile"));
  }

  // cache formatnames and format specifications for quick lookup
  QStringList formatnames;
  QList<KLFExportTypeUserScriptInfo::Format> formatlist;

  void clear()
  {
    // clear all properties
    QList<int> idlist = registeredPropertyIdList();
    for (int k = 0; k < idlist.size(); ++k) {
      setProperty(idlist[k], QVariant());
    }
    // and cached values
    formatnames = QStringList();
    formatlist = QList<KLFExportTypeUserScriptInfo::Format>();
  }

  void _set_xml_parsing_error(const QString& errmsg)
  {
    K->setScriptInfoError(1001, QString("Error parsing klf-export-type XML config: %1: %2")
                          .arg(K->userScriptBaseName()).arg(errmsg));
  }
  void parse_category_config(const QByteArray & ba)
  {
    QDomDocument doc("klf-export-type");
    QString errMsg; int errLine, errCol;
    bool r = doc.setContent(ba, false, &errMsg, &errLine, &errCol);
    if (!r) {
      K->setScriptInfoError(
          1001,
          QString("XML parse error: %1 (klf-export-type in %2, relative line %3 col %4)")
          .arg(errMsg).arg(K->userScriptBaseName()).arg(errLine).arg(errCol));
      return;
    }

    QDomElement root = doc.documentElement();
    if (root.nodeName() != "klf-export-type") {
      _set_xml_parsing_error(QString("expected <klf-export-type> element"));
      return;
    }
    
    // clear all properties
    clear();

    QVariantList formatsvlist;

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
      if (e.nodeName() == "input-data-type") {
        if (!property(KLFExportTypeUserScriptInfo::InputDataType).toString().isEmpty()) {
          _set_xml_parsing_error(QString("duplicate <input-data-type> element"));
          return;
        }
        setProperty(KLFExportTypeUserScriptInfo::InputDataType, val);
      } else if (e.nodeName() == "has-stdout-output") {
        if (!property(KLFExportTypeUserScriptInfo::HasStdoutOutput).toString().isEmpty()) {
          _set_xml_parsing_error(QString("duplicate <has-stdout-output> element"));
          return;
        }
        setProperty(KLFExportTypeUserScriptInfo::HasStdoutOutput,
                    klfLoadVariantFromText(val.toLatin1(), "bool").toBool());
      } else if (e.nodeName() == "mime-export-profiles-xml-file") {
        if (!property(KLFExportTypeUserScriptInfo::MimeExportProfilesXmlFile).toString().isEmpty()) {
          _set_xml_parsing_error(QString("duplicate <mime-export-profiles-xml-file> element"));
          return;
        }
        setProperty(KLFExportTypeUserScriptInfo::MimeExportProfilesXmlFile, val);
      } else if (e.nodeName() == "format") {
        // parse a new supported format
        if (!e.hasAttribute("name")) {
          _set_xml_parsing_error(QString("<format> tag without mandatory name=\"...\" attribute"));
          return;
        }
        QString formatName;
        QString formatTitle;
        QStringList fileNameExtensions;
        formatName = e.attribute("name");
        QDomNode n2;
        for (n2 = e.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          // try to convert the node to an element; ignore non-elements
          if ( n2.nodeType() != QDomNode::ElementNode ) {
            continue;
          }
          QDomElement e2 = n2.toElement();
          if ( e2.isNull() ) {
            continue;
          }
          QString val = e2.text();
          if (e2.nodeName() == "title") {
            formatTitle = val.trimmed();
          } else if (e2.nodeName() == "file-name-extension") {
            fileNameExtensions.append(val.trimmed());
          } else {
            _set_xml_parsing_error(QString("Unexpected node <%1> in <format> (name=%2)")
                                   .arg(e2.nodeName(),formatName));
            return;
          }
        }
        // record this available format
        formatnames.append(formatName);
        KLFExportTypeUserScriptInfo::Format fmt(formatName, formatTitle, fileNameExtensions);
        formatlist.append(fmt);
        formatsvlist.append(klfSave(&fmt));

      } else {
        _set_xml_parsing_error(QString("Found unexpected element: %1").arg(e.nodeName()));
        return;
      }

    }

    setProperty(KLFExportTypeUserScriptInfo::Formats, formatsvlist);

    klfDbg("Read all klfbackend-engine properties:\n" << qPrintable(toString()));
  }

};


KLFExportTypeUserScriptInfo::Format::Format()
  : KLFPropertizedObject("KLFExportTypeUserScriptInfo::Format")
{
  registerBuiltInProperty(FormatName, "FormatName");
  registerBuiltInProperty(FormatTitle, "FormatTitle");
  registerBuiltInProperty(FileNameExtensions, "FileNameExtensions");
  setProperty(FormatName, QString());
  setProperty(FormatTitle, QString());
  setProperty(FileNameExtensions, QStringList());
}
KLFExportTypeUserScriptInfo::Format::Format(const QString& formatName_, const QString& formatTitle_,
                                            const QStringList& fileNameExtensions_)
  : KLFPropertizedObject("KLFExportTypeUserScriptInfo::Format")
{
  registerBuiltInProperty(FormatName, "FormatName");
  registerBuiltInProperty(FormatTitle, "FormatTitle");
  registerBuiltInProperty(FileNameExtensions, "FileNameExtensions");
  setProperty(FormatName, formatName_);
  setProperty(FormatTitle, formatTitle_);
  setProperty(FileNameExtensions, fileNameExtensions_);
}
KLFExportTypeUserScriptInfo::Format::~Format()
{
}
QString KLFExportTypeUserScriptInfo::Format::formatName() const { return property(FormatName).toString(); }
QString KLFExportTypeUserScriptInfo::Format::formatTitle() const { return property(FormatTitle).toString(); }
QStringList KLFExportTypeUserScriptInfo::Format::fileNameExtensions() const
{ return property(FileNameExtensions).toStringList(); }



KLFExportTypeUserScriptInfo::KLFExportTypeUserScriptInfo(const QString& uspath)
  : KLFUserScriptInfo(uspath)
{
  KLF_INIT_PRIVATE( KLFExportTypeUserScriptInfo ) ;

  if (category() != "klf-export-type") {
    klfWarning("KLFExportTypeUserScriptInfo instantiated for user script "
               << uspath << ", which is of category " << category()) ;
  } else {
    d->parse_category_config(categorySpecificXmlConfig());
  }
}

KLFExportTypeUserScriptInfo::~KLFExportTypeUserScriptInfo()
{
  KLF_DELETE_PRIVATE ;
}



QStringList KLFExportTypeUserScriptInfo::formats() const
{
  return d->formatnames;
}
QString KLFExportTypeUserScriptInfo::inputDataType() const
{
  return klfExportTypeInfo(InputDataType).toString();
}
bool KLFExportTypeUserScriptInfo::hasStdoutOutput() const
{
  return klfExportTypeInfo(HasStdoutOutput).toBool();
}
QString KLFExportTypeUserScriptInfo::mimeExportProfilesXmlFile() const
{
  return klfExportTypeInfo(MimeExportProfilesXmlFile).toString();
}

QList<KLFExportTypeUserScriptInfo::Format> KLFExportTypeUserScriptInfo::formatList() const
{
  return d->formatlist;
}
KLFExportTypeUserScriptInfo::Format KLFExportTypeUserScriptInfo::getFormat(const QString& formatName) const
{
  for (int i = 0; i < d->formatlist.size(); ++i) {
    if (d->formatlist[i].formatName() == formatName) {
      return d->formatlist[i];
    }
  }
  klfWarning("Invalid format name: " << formatName) ;
  return KLFExportTypeUserScriptInfo::Format();
}


QVariant KLFExportTypeUserScriptInfo::klfExportTypeInfo(int propId) const
{
  return d->property(propId);
}

QVariant KLFExportTypeUserScriptInfo::klfExportTypeInfo(const QString& field) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QString x = field;

  klfDbg("x="<<x) ;
  int id = d->propertyIdForName(x);
  if (id < 0) {
    klfDbg("KLFExportTypeUserScriptInfo for "<<userScriptName()<<" does not have any information about "
           <<field<<" ("<<x<<")") ;
    return QVariant();
  }
  return scriptInfo(id);
}

QStringList KLFExportTypeUserScriptInfo::klfExportTypeInfosList() const
{
  return d->propertyNameList();
}




// -----------------------------------------------------------------------------


typedef QPair<QRegExp, QString> ReplPair;

static const QList<ReplPair > replace_display_latex = QList<ReplPair >()
  // LaTeX fine-tuning spacing commands \, \; \: \!
  << ReplPair(QRegExp("\\\\[,;:!]"), QLatin1String(""))
  // {\text{...}} and {\mathrm{...}}  -->  ...
  << ReplPair(QRegExp("\\{\\\\(?:text|mathrm)\\{((?:\\w|\\s|[._-])*)\\}\\}"),
                             QLatin1String("{\\1}"))
  // \text{...} and \mathrm{...}  -->  ...
  << ReplPair(QRegExp("\\\\(?:text|mathrm)\\{((?:\\w|\\s|[._-])*)\\}"), QLatin1String("{\\1}"))
  // \var(epsilon|phi|...)   -->  \epsilon, \phi
  << ReplPair(QRegExp("\\\\var([a-zA-Z]+)"), QLatin1String("\\\\1"))
     ;


QString klfLatexToPseudoTex(QString latex)
{
  // remove initial comments from latex code...
  QStringList latexlines = latex.split("\n");
  while (latexlines.size() && QRegExp("\\s*\\%.*").exactMatch(latexlines[0])) {
    latexlines.removeAt(0);
  }
  latex = latexlines.join("\n");

  // and simplify the LaTeX code a bit

  // format LaTeX code nicely to be displayed
  foreach (ReplPair rpl, replace_display_latex) {
    latex.replace(rpl.first, rpl.second);
  }

  return latex;  
}





