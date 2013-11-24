/***************************************************************************
 *   file klfbackend_p.h
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

#ifndef KLFBACKEND_P_H
#define KLFBACKEND_P_H

#include <stdlib.h>

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QtGlobal>




#include "klffilterprocess.h"

struct KLFBackendFilterProgram : public KLFFilterProcess
{
  int resErrCodes[KLFFP_PAST_LAST_VALUE];

  KLFBackendFilterProgram(const QString& title, const KLFBackend::klfSettings * settings,
                          bool isMainThread, const QString& rundir)
    : KLFFilterProcess(title, settings, rundir)
  {
    for (int i = 0; i < KLFFP_PAST_LAST_VALUE; ++i) {
      resErrCodes[i] = i;
    }

    setProcessAppEvents(isMainThread);
  }


  void errorToOutput(KLFBackend::klfOutput * resError)
  {
    if (resultStatus() == 0) {
      klfDbg("No error.") ;
    }
    resError->status = resErrCodes[resultStatus()];
    resError->errorstr = resultErrorString();
  }
};

















class KLFBackendTempDir
{
public:
  KLFBackendTempDir(const QString& templateName)
    : dirName()
  {
    QByteArray buffer = QFile::encodeName(templateName);
    if (!buffer.endsWith("XXXXXX"))
      buffer += "XXXXXX";
    if (q_mkdtemp(buffer.data())) { // modifies buffer
      dirName = QFile::decodeName(buffer.constData());
    }
  }

  ~KLFBackendTempDir()
  {
    const char *skipcleanup = getenv("KLFBACKEND_LEAVE_TEMP_FILES");
    if (skipcleanup != NULL && (*skipcleanup == '1' || *skipcleanup == 't' || *skipcleanup == 'T' ||
                                *skipcleanup == 'y' || *skipcleanup == 'Y' ||
                                QString::fromLatin1(skipcleanup).toLower() == QLatin1String("on"))) {
      return; // skip cleaning up temp directory
    }
    if (!dirName.size())
      return;

    Q_ASSERT(dirName != QLatin1String("."));

    const bool result = removeDirRecursively(dirName);
    if (!result) {
      klfWarning("Unable to remove"
                 << QDir::toNativeSeparators(dirName)
                 << "most likely due to the presence of read-only files.");
    }
  }


  QString path() const {
    return dirName;
  }

  QString tempFileInDir(const QString& filename) const {
    return dirName + "/" + filename;
  }


private:
  KLFBackendTempDir(const KLFBackendTempDir&) {}

  QString dirName;


  static bool removeDirRecursively(const QString& dirPath)
  {
    if (!dirPath.size())
      return true;

    bool success = true;
    QDir thedir = QDir(dirPath);
    // not empty -- we must empty it first
    QDirIterator di(dirPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    while (di.hasNext()) {
      di.next();
      const QFileInfo& fi = di.fileInfo();
      bool ok;
      if (fi.isDir() && !fi.isSymLink())
        ok = removeDirRecursively(di.filePath()); // recursive
      else
        ok = QFile::remove(di.filePath());
      if (!ok)
        success = false;
    }
    if (success)
      success = thedir.rmdir(thedir.absolutePath());
    return success;
  }


  /* Taken from Qt 5: Source of file QTemporaryDir. See
   * https://qt.gitorious.org/qt/qtbase/source/06ff9c9ec0e75eca2f06c177f16c52e42f41befd:src/corelib/io/qtemporarydir.cpp
   */
  static char *q_mkdtemp(char *templateName)
  {
#if defined(Q_OS_QNX ) || defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t length = strlen(templateName);
    char *XXXXXX = templateName + length - 6;
    if ((length < 6u) || strncmp(XXXXXX, "XXXXXX", 6))
      return 0;
    for (int i = 0; i < 256; ++i) {
      int v = qrand();
      /* Fill in the random bits. */
      XXXXXX[0] = letters[v % 62];
      v /= 62;
      XXXXXX[1] = letters[v % 62];
      v /= 62;
      XXXXXX[2] = letters[v % 62];
      v /= 62;
      XXXXXX[3] = letters[v % 62];
      v /= 62;
      XXXXXX[4] = letters[v % 62];
      v /= 62;
      XXXXXX[5] = letters[v % 62];
      QString templateNameStr = QFile::decodeName(templateName);
      QFileSystemEntry fileSystemEntry(templateNameStr);
      if (QFileSystemEngine::createDirectory(fileSystemEntry, false)) {
        QSystemError error;
        QFileSystemEngine::setPermissions(fileSystemEntry,
                                          QFile::ReadOwner |
                                          QFile::WriteOwner |
                                        QFile::ExeOwner, error);
        if (error.error() != 0)
        continue;
        return templateName;
      }
    }
    return 0;
#else
    return mkdtemp(templateName);
#endif
  }

};




#endif
