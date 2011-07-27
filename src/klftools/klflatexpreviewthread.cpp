/***************************************************************************
 *   file klflatexpreviewthread.cpp
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

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <klfbackend.h>

#include "klflatexpreviewthread.h"



struct KLFLatexPreviewThreadPrivate
{
  KLF_PRIVATE_HEAD(KLFLatexPreviewThread)
  {
    input = KLFBackend::klfInput();
    settings = KLFBackend::klfSettings();
    previewSize = QSize(280, 80);
    largePreviewSize = QSize(640, 480);

    hasnewinfo = false;
    abort = false;
  }

  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  QSize previewSize;
  QSize largePreviewSize;

  QWaitCondition condnewinfoavail;

  bool hasnewinfo;
  bool abort;

  // NOTE: main-thread/main-class has to mutex-lock at beginning of its method
  // already to access its 'd'-private-object !
  template<class T>
  bool mainThreadNewInfo(T * where, const T& what)
  {
    if (*where == what)
      return false;
    *where = what;
    hasnewinfo = true;
    condnewinfoavail.wakeOne();
    return true;
  }
};



KLFLatexPreviewThread::KLFLatexPreviewThread(QObject *parent)
  : QThread(parent)
{
  KLF_INIT_PRIVATE(KLFLatexPreviewThread) ;

  // we need to register meta-type KLFBackend::klfOutput for our signal, so register it if not yet done
  if (QMetaType::type("KLFBackend::klfOutput") == 0) {
    qRegisterMetaType<KLFBackend::klfOutput>("KLFBackend::klfOutput") ;
  }
}

KLFLatexPreviewThread::~KLFLatexPreviewThread()
{
  stop();

  KLF_DELETE_PRIVATE ;
}

KLF_DEFINE_PROPERTY_GET(KLFLatexPreviewThread, QSize, previewSize, PreviewSize) ;

KLF_DEFINE_PROPERTY_GET(KLFLatexPreviewThread, QSize, largePreviewSize, PreviewSize) ;


void KLFLatexPreviewThread::stop()
{
  // tell thread to stop, and wait for it
  _mutex.lock();
  d->abort = true;
  d->condnewinfoavail.wakeOne();
  _mutex.unlock();
  wait();
}


void KLFLatexPreviewThread::run()
{
  KLFBackend::klfInput ourinput;
  KLFBackend::klfSettings oursettings;
  KLFBackend::klfOutput ouroutput;
  QSize ps, lps;

  for (;;) {
    _mutex.lock();
    bool abrt = d->abort;
    _mutex.unlock();
    if (abrt)
      return;
  
    // fetch info
    _mutex.lock();
    ourinput = d->input;
    oursettings = d->settings;
    ps = d->previewSize;
    lps = d->largePreviewSize;
    d->hasnewinfo = false;
    _mutex.unlock();
    // render equation
    //  no performance improvement noticed with lower DPI:
    //    // force 240 DPI (we're only a preview...)
    //    input.dpi = 240;

    QImage img, prev, lprev;
    if ( ourinput.latex.trimmed().isEmpty() ) {
      QMetaObject::invokeMethod(this, "previewReset", Qt::QueuedConnection);
    } else {
      // and GO!
      ouroutput = KLFBackend::getLatexFormula(ourinput, oursettings);
      img = ouroutput.result;

      _mutex.lock();
      abrt = d->abort;
      bool hasnewinfo = d->hasnewinfo;
      _mutex.unlock();

      if (abrt)
        return;
      if (hasnewinfo)
        continue;

      if (ouroutput.status != 0) {
        // error... just debug.
        klfDbg("Real-Time preview failed; status="<<ouroutput.status<<".\n"<<ouroutput.errorstr) ;
        QMetaObject::invokeMethod(this, "previewError", Qt::QueuedConnection, Q_ARG(QString, ouroutput.errorstr),
			    Q_ARG(int, ouroutput.status));
      } else {
	QMetaObject::invokeMethod(this, "outputAvailable", Qt::QueuedConnection, Q_ARG(KLFBackend::klfOutput, ouroutput));
	if (ps.isValid()) {
	  prev = img;
	  if (prev.width() > ps.width() || prev.height() > ps.height())
	    prev = img.scaled(ps, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	if (lps.isValid()) {
	  lprev = img;
	  if (lprev.width() > lps.width() || lprev.height() > lps.height())
	    lprev = img.scaled(lps, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	QMetaObject::invokeMethod(this, "previewAvailable", Qt::QueuedConnection, Q_ARG(QImage, prev),
				  Q_ARG(QImage, lprev), Q_ARG(QImage, img));
	if (ps.isValid())
	  QMetaObject::invokeMethod(this, "previewImageAvailable", Qt::QueuedConnection, Q_ARG(QImage, prev));
	if (lps.isValid())
	  QMetaObject::invokeMethod(this, "previewLargeImageAvailable", Qt::QueuedConnection, Q_ARG(QImage, lprev));
	QMetaObject::invokeMethod(this, "previewFullImageAvailable", Qt::QueuedConnection, Q_ARG(QImage, img));
      }
    }

    _mutex.lock();
    d->condnewinfoavail.wait(&_mutex);
    _mutex.unlock();
  }
}



bool KLFLatexPreviewThread::setInput(const KLFBackend::klfInput& input)
{
  QMutexLocker mutexlocker(&_mutex);
  d->mainThreadNewInfo(&d->input, input);
}
bool KLFLatexPreviewThread::setSettings(const KLFBackend::klfSettings& settings, bool disableExtraFormats/* = true*/)
{
  KLFBackend::klfSettings s = settings;
  if (disableExtraFormats) {
    s.wantRaw = false;
    s.wantPDF = false;
    s.wantSVG = false;
  }

  QMutexLocker mutexlocker(&_mutex);
  d->mainThreadNewInfo(&d->settings, s);
}

bool KLFLatexPreviewThread::setPreviewSize(const QSize& previewSize)
{
  QMutexLocker mutexlocker(&_mutex);
  d->mainThreadNewInfo(&d->previewSize, previewSize);
}
bool KLFLatexPreviewThread::setLargePreviewSize(const QSize& largePreviewSize)
{
  QMutexLocker mutexlocker(&_mutex);
  d->mainThreadNewInfo(&d->largePreviewSize, largePreviewSize);
}



