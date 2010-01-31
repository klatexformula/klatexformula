/***************************************************************************
 *   file klflib.h
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

#ifndef KLFLIB_H
#define KLFLIB_H

#include <QImage>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <klfdata.h>  // KLFStyle
#include <klfpobj.h>

/** \brief An entry (single formula) in the library
 */
class KLF_EXPORT KLFLibEntry : public KLFPropertizedObject {
public:
  /** \note The numeric IDs don't have to be preserved from one version of KLF to
   * another, since they are nowhere stored. Properties are always stored by name
   * when dealing in scopes larger than the running application (saved files, etc.). */
  enum PropertyType { Latex, //!< The Latex Code of the equation
		      DateTime, //!< The Date/Time at which the equation was evaluated
		      Preview, //!< An Image Preview of equation (scaled down QImage)
		      Category, //!< The Category to which eq. belongs (path-style string)
		      Tags, //!< Tags about the equation (string)
		      Style //!< KLFStyle style used
  };

  KLFLibEntry(const QString& latex = QString(), const QDateTime& dt = QDateTime(),
	      const QImage& preview = QImage(),	const QString& category = QString(),
	      const QString& tags = QString(), const KLFStyle& style = KLFStyle());
  KLFLibEntry(const KLFLibEntry& copy);
  virtual ~KLFLibEntry();

  QString latex() const { return property(Latex).toString(); }
  QDateTime dateTime() const { return property(DateTime).toDateTime(); }
  QImage preview() const { return property(Preview).value<QImage>(); }
  QString category() const { return property(Category).toString(); }
  QString tags() const { return property(Tags).toString(); }
  KLFStyle style() const { return property(Style).value<KLFStyle>(); }

  void setLatex(const QString& latex) { setProperty(Latex, latex); }
  void setDateTime(const QDateTime& dt) { setProperty(DateTime, dt); }
  void setPreview(const QImage& img) { setProperty(Preview, img); }
  void setCategory(const QString& s) { setProperty(Category, s); }
  void setTags(const QString& s) { setProperty(Tags, s); }
  void setStyle(const KLFStyle& style) { setProperty(Style, style); }

private:

  static void initRegisteredProperties();
};


class KLF_EXPORT KLFLibResourceEngine : public QObject
{
  Q_OBJECT
public:
  typedef int entryId;

  struct KLFLibEntryWithId {
    entryId id;
    KLFLibEntry data;
  };

  KLFLibResourceEngine();
  virtual ~KLFLibResourceEngine();

  virtual KLFLibEntry entry(entryId id) = 0;
  virtual QList<KLFLibEntryWithId> allEntries() = 0;

  virtual entryId insertEntry(const KLFLibEntry& entry) = 0;
  virtual bool deleteEntry(const entryId& id) = 0;
};


/** Library Resource engine implementation for an (abstract) database (using Qt
 * SQL interfaces) */
class KLF_EXPORT KLFLibDBEngine : public KLFLibResourceEngine
{
public:
  KLFLibDBEngine(QSqlDatabase db_connection = QSqlDatabase());
  virtual ~KLFLibDBEngine();

  /** True if one has supplied a valid database in the constructor or with a
   * \ref setDatabse() call. */
  bool validDatabase() const;
  /** supply an open database. */
  void setDatabase(QSqlDatabase db_connection);

  virtual KLFLibEntry entry(entryId id);
  virtual QList<KLFLibEntryWithId> allEntries();

  virtual entryId insertEntry(const KLFLibEntry& entry);
  virtual bool deleteEntry(const entryId& id);

  static bool initFreshDatabase(QSqlDatabase db);

private:
  QSqlDatabase pDB;

  QMap<int,int> detectEntryColumns(const QSqlQuery& q);
  KLFLibEntry readEntry(const QSqlQuery& q, QMap<int,int> columns);
};






class KLFLibLegacyLibraryEngine : public KLFLibResourceEngine
{
  // ..........
};

#endif

