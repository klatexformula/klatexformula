/***************************************************************************
 *   file klflatexpreviewthread_p.h
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

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflatexpreviewthread.cpp */

#ifndef KLFLATEXPREVIEWTHREAD_P_H
#define KLFLATEXPREVIEWTHREAD_P_H

#include "klflatexpreviewthread.h"



class KLFContLatexPreviewPrivate : public KLFLatexPreviewHandler
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFContLatexPreview, KLFLatexPreviewHandler)
  {
    thread = NULL;

    input = KLFBackend::klfInput();
    settings = KLFBackend::klfSettings();
    previewSize = QSize(280, 80);
    largePreviewSize = QSize(640, 480);
  }
  virtual ~KLFContLatexPreviewPrivate()
  {
  }

  KLFLatexPreviewThread * thread;

  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  QSize previewSize;
  QSize largePreviewSize;

  void refreshPreview()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    KLF_ASSERT_NOT_NULL(thread, "Thread is NULL! Can't refresh preview!", return; ) ;

    bool ok = thread->clearAndSubmitPreviewTask(input, settings, this, previewSize, largePreviewSize);
    if (!ok) {
      klfWarning("Failed to submit preview task to thread.") ;
    }
  }

public slots:

  void latexPreviewReset()
  {
    emit K->previewReset();
  }

  void latexOutputAvailable(const KLFBackend::klfOutput& output)
  {
    emit K->outputAvailable(output);
  }
  void latexPreviewAvailable(const QImage& preview, const QImage& largePreview, const QImage& fullPreview)
  {
    emit K->previewAvailable(preview, largePreview, fullPreview);
  }
  void latexPreviewImageAvailable(const QImage& preview)
  {
    emit K->previewImageAvailable(preview);
  }
  void latexPreviewLargeImageAvailable(const QImage& largePreview)
  {
    emit K->previewLargeImageAvailable(largePreview);
  }
  void latexPreviewFullImageAvailable(const QImage& fullPreview)
  {
    emit K->previewFullImageAvailable(fullPreview);
  }

  void latexPreviewError(const QString& errorString, int errorCode)
  {
    emit K->previewError(errorString, errorCode);
  }
};






#endif


