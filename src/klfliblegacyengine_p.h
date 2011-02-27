/***************************************************************************
 *   file klfliblegacyengine_p.h
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


/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klfliblegacyengine.cpp */

#ifndef KLFLIBLEGACYENGINE_P_H
#define KLFLIBLEGACYENGINE_P_H

#include <QObject>
#include <QMap>
#include <QFileInfo>

#include "klfliblegacyengine.h"

/** \internal */
class KLFLibLegacyFileDataPrivate : public QObject
{
  Q_OBJECT
public:
  /** Get the KLFLibLegacyFileDataPrivate instance assigned to file \c fname. The file data is already
   * loaded, ie. don't call load().
   *
   * Don't forget to call ref() to reference the returned object.
   *
   * \c autoSaveTimer is instantiated and started; however you must 
   */
  static inline KLFLibLegacyFileDataPrivate * instanceFor(const QString fname, bool starttimer)
  {
    QString f = canonicalFilePath(fname);
    klfDbg("fname="<<fname<<"; canonical f="<<f<<"; starttimer="<<starttimer) ;
    if (f.isEmpty()) {
      qWarning()<<KLF_FUNC_NAME<<": error getting canonical file path for "<<fname<<".";
      return NULL;
    }
    if (staticFileDataObjects.contains(f))
      return staticFileDataObjects[f];
    KLFLibLegacyFileDataPrivate *d = new KLFLibLegacyFileDataPrivate(f);
    if (starttimer && !d->autoSaveTimer->isActive())
      d->autoSaveTimer->start(180000); // 180s = 3min
    return d;
  }

  /** Returns a path that will be "canonicalized", ie. two (string-wise) different paths pointing to the
   * same file will have the same canonical path (eg. '..' entries simplified, symlinks resolved).
   *
   * Works for both existing and non-existing files. However for non-existing files the containing directory
   * must exist.
   *
   * An empty string is returned to indicate an error (eg. containing directory does not exist).
   */
  static QString canonicalFilePath(const QString& fname)
  {
    QFileInfo fi(fname);
    if (fi.exists())
      return fi.canonicalFilePath();
    // non-existing file. Rely on existing directory
    QString containdir = fi.absolutePath();
    klfDbg("non-existing file "<<fname<<": containing dir="<<containdir) ;
    QFileInfo di(containdir);
    if (!di.exists() || !di.isDir()) {
      qWarning()<<KLF_FUNC_NAME<<": Path "<<fname<<": directory "<<containdir<<" does not exist.";
      return QString();
    }
    QString canonical = QFileInfo(containdir).canonicalFilePath();
    if (canonical.isEmpty()) {
      qWarning()<<KLF_FUNC_NAME<<": Error getting "<<containdir<<"'s canonical path.";
      return QString();
    }
    if (!canonical.endsWith("/"))
      canonical += "/";
    canonical += fi.fileName();
    return canonical;
  }

  /** Saves the file, removes this instance from the static instance list and deletes the timer. */
  ~KLFLibLegacyFileDataPrivate()
  {
    klfDbg("destroying. Possibly save? haschanges="<<haschanges) ;
    if (haschanges)
      save();

    staticFileDataObjects.remove(filename);
    delete autoSaveTimer;
  }

  /** Should be called explicitely by any class that wishes to use this instance. See also \ref deref() */
  inline void ref() { ++refcount; }
  /** Dereferences, and returns the remaining number of references to this object. The caller should
   * delete the object if the return value is zero. */
  inline int deref() { return --refcount; }

  inline QString fileName() const { return filename; }


  enum LegacyLibType { LocalHistoryType = 1, LocalLibraryType, ExportLibraryType };

  bool haschanges;

  /** upon modification, DON'T FORGET to set \ref haschanges ! */
  KLFLegacyData::KLFLibrary library;
  /** upon modification, DON'T FORGET to set \ref haschanges ! */
  KLFLegacyData::KLFLibraryResourceList resources;

  /** Metadata, may be used for any purpose.
   *
   * upon modification, DON'T FORGET to set \ref haschanges !
   *
   * List of properties:
   *  - \c "ResProps" : a QVariantMap with all resource properties as { 'name' => value }
   *
   * \todo In the future, this will be how resource and sub-resource properties will be supported.
   */
  QVariantMap metadata;

  LegacyLibType legacyLibType;

  QTimer *autoSaveTimer;

  /** Returns the index in \ref resources */
  int findResourceName(const QString& resname);
  int getReservedResourceId(const QString& resourceName, int defaultId);



  static inline KLFLibEntry toLibEntry(const KLFLegacyData::KLFLibraryItem& item)
  {
    return KLFLibEntry(KLFLibEntry::stripCategoryTagsFromLatex(item.latex), item.datetime,
		       item.preview.toImage(), item.preview.size(), item.category,
		       item.tags, toStyle(item.style));
  }
  static inline KLFLegacyData::KLFLibraryItem toLegacyLibItem(const KLFLibEntry& entry)
  {
    KLFLegacyData::KLFLibraryItem item;
    item.id = KLFLegacyData::KLFLibraryItem::MaxId++;
    // ensure latex has category & tags information
    item.latex = KLFLibEntry::latexAddCategoryTagsComment(KLFLibEntry::stripCategoryTagsFromLatex(entry.latex()),
							  entry.category(), entry.tags()) ;
    item.category = entry.category();
    item.tags = entry.tags();
    item.preview = QPixmap::fromImage(entry.preview());
    item.datetime = entry.dateTime();
    item.style = toLegacyStyle(entry.style());
    return item;
  }
  static inline KLFLegacyData::KLFStyle toLegacyStyle(const KLFStyle& style)
  {
    KLFLegacyData::KLFStyle oldstyle;
    oldstyle.name = style.name;
    oldstyle.fg_color = style.fg_color;
    oldstyle.bg_color = style.bg_color;
    oldstyle.mathmode = style.mathmode;
    oldstyle.preamble = style.preamble;
    oldstyle.dpi = style.dpi;
    return oldstyle;
  }
  static inline KLFStyle toStyle(const KLFLegacyData::KLFStyle& oldstyle)
  {
    KLFStyle style;
    style.name = oldstyle.name;
    style.fg_color = oldstyle.fg_color;
    style.bg_color = oldstyle.bg_color;
    style.mathmode = oldstyle.mathmode;
    style.preamble = oldstyle.preamble;
    style.dpi = oldstyle.dpi;
    return style;
  }

signals:
  void resourcePropertyChanged(int propId);

public slots:
  /** Loads the current object from the file.
   *
   * \warning calling this function is dangerous, as you can override changes done by another object
   *   to \ref library and \ref resources.  */
  bool load(const QString& fname = QString());

  /** Saves the current object to the file */
  bool save(const QString& fname = QString());

  void emitResourcePropertyChanged(int propId) { emit resourcePropertyChanged(propId); }

private:
  KLFLibLegacyFileDataPrivate() { }

  KLFLibLegacyFileDataPrivate(const QString& fname) : refcount(0), filename(fname)
  {
    klfDbg(" filename is "<<filename ) ;

    staticFileDataObjects[filename] = this;

    if (QFile::exists(fname))
      load(); // load the data

    // by default, we're a .klf export type
    legacyLibType = ExportLibraryType;

    // prepare the autosave timer
    autoSaveTimer = new QTimer(NULL);
    autoSaveTimer->setSingleShot(false);
    connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
  }

  int refcount;

  QString filename;

  static QMap<QString,KLFLibLegacyFileDataPrivate*> staticFileDataObjects;

};




#endif
