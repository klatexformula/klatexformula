/***************************************************************************
 *   file klflibwikiscan.h
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

#ifndef KLFLIBWIKISCAN_H
#define KLFLIBWIKISCAN_H

#include <QtNetwork>

#include <klfdefs.h>
#include <klflib.h>



/** Library Resource engine implementation for accessing formulas on wikipedia pages
 *
 *
 * note:  "sub-resource" = wiki page name
 */
class KLF_EXPORT KLFLibWikiScanEngine : public KLFLibResourceEngine
{
  Q_OBJECT

public:
  /** Use this function as a constructor. Creates a KLFLibWikiScanEngine object,
   * with QObject parent \c parent, opening the wiki page \c url.
   * Returns NULL if opening the database failed.
   *
   * \note the URL \c url is a klflib-url, it has to have the schema klf+wikiscan://
   *   instead of the usual http:// prefix.
   *
   * A non-NULL returned object successfully requested the page. The reply may not
   * have been received yet, though, in which case attepts to read will block.
   * */
  static KLFLibWikiScanEngine * openUrl(const QUrl& url, QObject *parent = NULL);

  /** Simple destructor. */
  virtual ~KLFLibWikiScanEngine();

  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xfffffff) const;

  virtual bool compareDefaultSubResourceEquals(const QString& subResourceName) const;

  virtual bool canModifyData(const QString& /*subRes*/, ModifyType /*modifytype*/) const
  { return false; }
  virtual bool canModifyProp(int /*propid*/) const
  { return false; }
  virtual bool canRegisterProperty(const QString& /*propName*/) const
  { return false; }


  //  virtual QByteArray contentHTML(const QString& subResource);


  virtual QList<KLFLib::entryId> allIds(const QString& subResource);
  virtual bool hasEntry(const QString&, entryId id);
  virtual QList<KLFLibEntryWithId> entries(const QString&, const QList<KLFLib::entryId>& idList,
					   const QList<int>& wantedEntryProperties = QList<int>());

  virtual int query(const QString& subResource, const Query& query, QueryResult *result);
  virtual QList<QVariant> queryValues(const QString& subResource, int entryPropId);

  virtual KLFLibEntry entry(const QString& subRes, entryId id);
  virtual QList<KLFLibEntryWithId> allEntries(const QString& subRes,
					      const QList<int>& wantedEntryProperties = QList<int>());

  virtual bool canCreateSubResource() const { return false; }
  virtual bool canRenameSubResource() const { return false; }
  virtual bool canDeleteSubResource(const QString& subResource) const { return false; }

  virtual QVariant subResourceProperty(const QString& subResource, int propId) const;

  virtual QList<int> subResourcePropertyIdList() const
  { return QList<int>() << SubResPropTitle << SubResPropViewType << SubResPropLocked; }

  virtual bool hasSubResource(const QString& subRes) const;
  virtual QStringList subResourceList() const;

public slots:

  //   virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle)
  //   virtual bool deleteSubResource(const QString& subResource)

  //   virtual QList<entryId> insertEntries(const QString& subRes, const KLFLibEntryList& entries)
  //   virtual bool changeEntries(const QString& subRes, const QList<entryId>& idlist,
  // 			     const QList<int>& properties, const QList<QVariant>& values)
  //   virtual bool deleteEntries(const QString& subRes, const QList<entryId>& idlist);
  
  //   virtual bool saveTo(const QUrl& newPath);
  
  //   virtual bool setSubResourceProperty(const QString& subResource, int propId, const QVariant& value);

protected:
  //  virtual bool saveResourceProperty(int propId, const QVariant& value);

private:
  KLFLibWikiScanEngine(const QString& wikipageurl, const QUrl& url,
		       bool accessshared, QObject *parent);

};



/** The associated \ref KLFLibEngineFactory factory to the \ref KLFLibWikiScanEngine resource engine. */
class KLF_EXPORT KLFLibWikiScanEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibDBEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibDBEngineFactory() { }

  virtual QStringList supportedTypes() const;
  virtual QString schemeTitle(const QString& scheme) const ;
  virtual uint schemeFunctions(const QString& scheme) const ;

  virtual QString correspondingWidgetType(const QString& scheme) const;
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);

  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);
};



#endif
