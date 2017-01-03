/***************************************************************************
 *   file klfwinclipboard.cpp
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

#include <cstdlib>
#include <cstring>

#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>
#include <QByteArray>

#include <windows.h>

#include <klfdefs.h>
#include <klfdebug.h>
#include <klfmime.h>
#include <mswin/klfwinclipboard.h>


struct FmtWithId {
  FmtWithId(const QString& fmt_ = QString(), int id_ = 0) : fmt(fmt_), id(id_) { }
  QString fmt;
  int id;
};

struct KLFWinClipboardPrivate {
  KLF_PRIVATE_HEAD(KLFWinClipboard) {
  }

  static int getStdWinFormatConstant(const QString & wfmtname);

  static QList<FmtWithId> registeredWinFormats;
};

// static
QList<FmtWithId> KLFWinClipboardPrivate::registeredWinFormats = QList<FmtWithId>();


KLFWinClipboard::KLFWinClipboard()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE(KLFWinClipboard) ;
}
KLFWinClipboard::~KLFWinClipboard()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_DELETE_PRIVATE ;
}
//static
int KLFWinClipboardPrivate::getStdWinFormatConstant(const QString & wfmtname)
{
  if (!wfmtname.startsWith("CF_")) {
    return -1;
  }
  if (wfmtname == QLatin1String("CF_BITMAP")) {
    return CF_BITMAP;
  } else if (wfmtname == QLatin1String("CF_BITMAP")) {
    return CF_BITMAP;
  } else if (wfmtname == QLatin1String("CF_DIB")) {
    return CF_DIB;
  } else if (wfmtname == QLatin1String("CF_DIBV5")) {
    return CF_DIBV5;
  } else if (wfmtname == QLatin1String("CF_ENHMETAFILE")) {
    return CF_ENHMETAFILE;
  } else if (wfmtname == QLatin1String("CF_METAFILEPICT")) {
    return CF_METAFILEPICT;
  } else if (wfmtname == QLatin1String("CF_OEMTEXT")) {
    return CF_OEMTEXT;
  } else if (wfmtname == QLatin1String("CF_TEXT")) {
    return CF_TEXT;
  } else if (wfmtname == QLatin1String("CF_UNICODETEXT")) {
    return CF_UNICODETEXT;
  }
  klfWarning("Format "<<wfmtname<<" looks like a predefined windows constant format type but "
             "it doesn't match those we know about! Is it a typo?")
  return -1;
}
bool KLFWinClipboard::canConvertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  const QString prefix = KLF_MIME_PROXY_WIN_FORMAT_PREFIX;

  foreach(QString mime, mimeData->formats()) {
    klfDbg("checking mime="<<mime) ;
    if (mime.startsWith(prefix)) {
      QString wfmt = mime.mid(prefix.length());
      klfDbg("wfmt = " << wfmt) ;
      if (KLFWinClipboardPrivate::getStdWinFormatConstant(wfmt) == formatetc.cfFormat) {
        // wfmt is a predefined 
        return true;
      }
      // find wfmt in registeredWinFormats and see if IDs match.
      foreach (FmtWithId rf, KLFWinClipboardPrivate::registeredWinFormats) {
        if (wfmt == rf.fmt && rf.id == formatetc.cfFormat) {
          return true;
        }
      }
    } else {
      klfDbg("Mime type is not a KLF win format proxy.") ;
    }
  }
  return false;
}
bool KLFWinClipboard::canConvertToMime(const QString &, IDataObject *) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return false;
}
bool KLFWinClipboard::convertFromMime(const FORMATETC &formatetc, const QMimeData *mimeData,
                                      STGMEDIUM *pmedium) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // convert. QWinMime docs are totally unclear about what to expect in the
  // structures. For now, let's only accept the tymed == TYMED_HGLOBAL .
  if (formatetc.tymed != TYMED_HGLOBAL) {
    klfWarning("Win clipboard data transfer other than TYMED_HGLOBAL not supported (formatetc.tymed="
               << formatetc.tymed << ")") ;
    return false;
  }

  const QString prefix = KLF_MIME_PROXY_WIN_FORMAT_PREFIX;

  // decide which format to use
  
  foreach (QString mime, mimeData->formats()) {

    // only continue if this is a proxy mime for a windows format
    if (!mime.startsWith(prefix)) {
      continue;
    }
    QString wfmt = mime.mid(prefix.length());

    // Get the windows ID of this format.  
    int fmt_id = -1;

    // First, see if it is a built-in windows format
    fmt_id = KLFWinClipboardPrivate::getStdWinFormatConstant(wfmt);

    if (fmt_id == -1) {
      // If it isn't a built-in windows format, it must have been previously registered
      // because it was (I hope!!!) obtained by formatsForMime()
      int k;
      for (k = 0; k < KLFWinClipboardPrivate::registeredWinFormats.size(); ++k) {
        const FmtWithId wfmtid = KLFWinClipboardPrivate::registeredWinFormats[k];
        if (wfmtid.fmt == wfmt) {
          // found
          fmt_id = wfmtid.id;
          break;
        }
      }
    }
    KLF_ASSERT_CONDITION(fmt_id != -1, "windows format " << wfmt << " not registered!!!!", return false; ) ;

    // get the data
    QByteArray data = mimeData->data(mime);

    // QWinMime docs are bad and give no clue as to how to set the data in pmedium, e.g.,
    // whether fields are prealllocated or not ... found an example in
    // qt-everywhere-opensource-src-5.7.1/qtbase/src/plugins/platforms/windows/qwindowsmime.cpp
    // It seems that nothing in pmedium is prepared and that we have to set all fields.

    HGLOBAL hData = GlobalAlloc(0, SIZE_T(data.size()));
    if (!hData) {
      klfWarning("Error: can't allocate HGLOBAL data");
        return false;
    }

    void *out = GlobalLock(hData);
    std::memcpy(out, data.data(), size_t(data.size()));
    GlobalUnlock(hData);
    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->hGlobal = hData;
    pmedium->pUnkForRelease = 0;
    return true;
  }
  return false;
}
QVariant KLFWinClipboard::convertToMime(const QString &, IDataObject *,
                                        QVariant::Type ) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return QVariant(); // don't convert anything from the Windows clipboard.
}
QVector<FORMATETC> KLFWinClipboard::formatsForMime(const QString &mimeType, const QMimeData */*mimeData*/) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QVector<FORMATETC> formats;

  const QString prefix = KLF_MIME_PROXY_WIN_FORMAT_PREFIX;

  // only continue if this is a proxy mime for a windows format
  if (!mimeType.startsWith(prefix)) {
    return formats;
  }
  QString wfmt = mimeType.mid(prefix.length());

  // see if this format is registered, and if necessary, register it.
  int k;
  int fmt_id = -1;
  for (k = 0; k < KLFWinClipboardPrivate::registeredWinFormats.size(); ++k) {
    const FmtWithId wfmtid = KLFWinClipboardPrivate::registeredWinFormats[k];
    if (wfmtid.fmt == wfmt) {
      // found
      fmt_id = wfmtid.id;
      break;
    }
  }
  if (fmt_id == -1) {
    // not found, needs to be registered
    //
    // ???: what's the argument of registerMimeType()?? QWinMime docs are unclear. I
    // imagine it's supposed to be the windows format name.
    fmt_id = QWinMime::registerMimeType(wfmt);
    KLFWinClipboardPrivate::registeredWinFormats.append(FmtWithId(wfmt, fmt_id));
  }
    
  // and add this format to our list
  FORMATETC winformat;
  winformat.cfFormat = fmt_id;
  winformat.ptd = NULL;
  winformat.dwAspect = DVASPECT_CONTENT;
  winformat.lindex = -1;
  winformat.tymed = TYMED_HGLOBAL;

  formats.append(winformat);

  return formats;
}
QString KLFWinClipboard::mimeForFormat(const FORMATETC &) const
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  return QString(); // no conversion in this direction
}




// void klfWinClipboardCopy(HWND h, const QStringList& wintypes, const QList<QByteArray>& datalist)
// {
//   if (wintypes.size() != datalist.size()) {
//     qWarning("win: Need same list sizes for wintypes and datalist! (got %d and %d resp.)",
// 	     (int)wintypes.size(), (int)datalist.size());
//     return;
//   }
//   if ( ! OpenClipboard(h) ) {
//     qWarning("win: Cannot open the Clipboard");
//     return;
//   }
//   // Remove the current Clipboard contents
//   if( ! EmptyClipboard() ) {
//     qWarning("win: Cannot empty the Clipboard");
//     return;
//   }
//   int k;
//   for (k = 0; k < wintypes.size(); ++k) {
//     const QString type = wintypes[k];
//     const char * data = datalist[k].constData();
//     const int size = datalist[k].size();
//     // Register required datatype
//     int wintype =
//       RegisterClipboardFormatA(type.toLocal8Bit());
//     if (!wintype) {
//       qWarning("win: Unable to register clipboard format `%s' !", qPrintable(type));
//       continue;
//     }
//     // Get the currently selected data
//     HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, size);
//     if ( ! hGlob ) {
//       qWarning("win: Unable to GlobalAlloc(): %d", (int)GetLastError());
//       continue;
//     }
//     char * ptr = (char*)GlobalLock(hGlob);
//     if (ptr == NULL) {
//       qWarning("win: Unable to GlobalLock(): %d", (int)GetLastError());
//       continue;
//     }
//     memcpy(ptr, data, size);
//     GlobalUnlock(hGlob);
//     // Set clipboard data for this type
//     if ( ::SetClipboardData( wintype, hGlob ) == NULL ) {
//       qWarning("win: Unable to set Clipboard data, error: %d", (int)GetLastError());
//       CloseClipboard();
//       GlobalFree(hGlob);
//       return;
//     }
//   }
//   // finally close clipboard, we're done.
//   CloseClipboard();
// }
