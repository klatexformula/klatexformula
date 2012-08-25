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

#include <QImage>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>

#include <klfbackend.h>

#include "klflatexpreviewthread.h"
#include "klflatexpreviewthread_p.h"



KLFLatexPreviewHandler::KLFLatexPreviewHandler(QObject * parent)
  : QObject(parent)
{
}
KLFLatexPreviewHandler::~KLFLatexPreviewHandler()
{
}

void KLFLatexPreviewHandler::latexPreviewReset()
{
}
void KLFLatexPreviewHandler::latexOutputAvailable(const KLFBackend::klfOutput& output)
{
  Q_UNUSED(output);
}
void KLFLatexPreviewHandler::latexPreviewAvailable(const QImage& preview, const QImage& largePreview,
						   const QImage& fullPreview)
{
  Q_UNUSED(preview); Q_UNUSED(largePreview); Q_UNUSED(fullPreview);
}
void KLFLatexPreviewHandler::latexPreviewImageAvailable(const QImage& preview)
{
  Q_UNUSED(preview);
}
void KLFLatexPreviewHandler::latexPreviewLargeImageAvailable(const QImage& largePreview)
{
  Q_UNUSED(largePreview);
}
void KLFLatexPreviewHandler::latexPreviewFullImageAvailable(const QImage& fullPreview)
{
  Q_UNUSED(fullPreview);
}
void KLFLatexPreviewHandler::latexPreviewError(const QString& errorString, int errorCode)
{
  Q_UNUSED(errorString); Q_UNUSED(errorCode);
}



// ---


KLFLatexPreviewThread::KLFLatexPreviewThread(QObject * parent)
  : QThread(parent)
{
  KLF_INIT_PRIVATE(KLFLatexPreviewThread) ;

  // we need to register meta-type KLFBackend::klfOutput for our signal/meta-object call, so register
  // it if not yet done
  if (QMetaType::type("KLFBackend::klfOutput") == 0) {
    qRegisterMetaType<KLFBackend::klfOutput>("KLFBackend::klfOutput") ;
  }
}

KLFLatexPreviewThread::~KLFLatexPreviewThread()
{
  stop();

  KLF_DELETE_PRIVATE ;
}

KLF_DEFINE_PROPERTY_GET(KLFLatexPreviewThread, QSize, previewSize) ;

KLF_DEFINE_PROPERTY_GET(KLFLatexPreviewThread, QSize, largePreviewSize) ;


void KLFLatexPreviewThread::stop()
{
  // tell thread to stop, and wait for it
  _mutex.lock();
  d->abort = true;
  d->condNewTaskAvailable.wakeOne();
  _mutex.unlock();
  wait();
}


void KLFLatexPreviewThread::run()
{
  KLFLatexPreviewThreadPrivate::Task task;
  bool abrt, hastask;
  KLFBackend::klfOutput ouroutput;

  for (;;) {
    _mutex.lock();
    abrt = d->abort;
    hastask = d->newTasks.size();
    _mutex.unlock();
    if (abrt)
      return;

    if (hastask) {
      // fetch task info
      _mutex.lock();
      task = d->newTasks.dequeue();
      QMetaObject::invokeMethod(d, "setTaskRunning", Qt::QueuedConnection,
				Q_ARG(void *, (void*)task.qtask));
      _mutex.unlock();
      // render equation
      //  no performance improvement noticed with lower DPI:
      //    // force 240 DPI (we're only a preview...)
      //    input.dpi = 240;

      QImage img, prev, lprev;
      if ( task.input.latex.trimmed().isEmpty() ) {
	QMetaObject::invokeMethod(task.handler, "latexPreviewReset", Qt::QueuedConnection);
      } else {
	// and GO!
	ouroutput = KLFBackend::getLatexFormula(task.input, task.settings);
	img = ouroutput.result;

	if (ouroutput.status != 0) {
	  // error...
	  QMetaObject::invokeMethod(task.handler, "latexPreviewError", Qt::QueuedConnection,
				    Q_ARG(QString, ouroutput.errorstr),
				    Q_ARG(int, ouroutput.status));
	} else {
	  QMetaObject::invokeMethod(task.handler, "latexOutputAvailable", Qt::QueuedConnection,
				    Q_ARG(KLFBackend::klfOutput, ouroutput));
	  if (task.previewSize.isValid()) {
	    prev = img;
	    if (prev.width() > task.previewSize.width() || prev.height() > task.previewSize.height())
	      prev = img.scaled(task.previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	  }
	  if (task.largePreviewSize.isValid()) {
	    lprev = img;
	    if (lprev.width() > task.largePreviewSize.width() || lprev.height() > task.largePreviewSize.height())
	      lprev = img.scaled(task.largePreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	  }
	  
	  QMetaObject::invokeMethod(task.handler, "latexPreviewAvailable", Qt::QueuedConnection,
				    Q_ARG(QImage, prev),
				    Q_ARG(QImage, lprev),
				    Q_ARG(QImage, img));
	  if (task.previewSize.isValid())
	    QMetaObject::invokeMethod(task.handler, "latexPreviewImageAvailable", Qt::QueuedConnection,
				      Q_ARG(QImage, prev));
	  if (task.largePreviewSize.isValid())
	    QMetaObject::invokeMethod(task.handler, "latexPreviewLargeImageAvailable", Qt::QueuedConnection,
				      Q_ARG(QImage, lprev));
	  QMetaObject::invokeMethod(task.handler, "latexPreviewFullImageAvailable", Qt::QueuedConnection,
				    Q_ARG(QImage, img));
	}
      }
    }
    
    _mutex.lock();
    QMetaObject::invokeMethod(d, "setTaskFinished", Qt::QueuedConnection,
			      Q_ARG(void *, (void*) task.qtask));
    abrt = d->abort;
    hastask = d->newTasks.size();
    _mutex.unlock();
    
    if (abrt)
      return;
    if (hastask)
      continue;
      
    _mutex.lock();
    d->condNewTaskAvailable.wait(&_mutex);
    _mutex.unlock();
  }
}


KLFRefPtr<KLFLatexPreviewThread::QueuedTask>
/* */  KLFLatexPreviewThread::submitPreviewTask(const KLFBackend::klfInput& input,
						const KLFBackend::klfSettings& settings,
						KLFLatexPreviewHandler * outputhandler,
						const QSize& previewSize,
						const QSize& largePreviewSize)
{
  QMutexLocker mutexlocker(&_mutex);

  KLFLatexPreviewThreadPrivate::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = previewSize;
  t.largePreviewSize = largePreviewSize;

  return d->submitTask(t);
}

KLFRefPtr<KLFLatexPreviewThread::QueuedTask>
/* */  KLFLatexPreviewThread::submitPreviewTask(const KLFBackend::klfInput& input,
						const KLFBackend::klfSettings& settings,
						KLFLatexPreviewHandler * outputhandler)
{
  QMutexLocker mutexlocker(&_mutex);

  KLFLatexPreviewThreadPrivate::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = d->previewSize;
  t.largePreviewSize = d->largePreviewSize;

  return d->submitTask(t);
}

KLFRefPtr<KLFLatexPreviewThread::QueuedTask>
/* */  KLFLatexPreviewThread::clearAndSubmitPreviewTask(const KLFBackend::klfInput& input,
							const KLFBackend::klfSettings& settings,
							KLFLatexPreviewHandler * outputhandler,
							const QSize& previewSize,
							const QSize& largePreviewSize)
{
  QMutexLocker mutexlocker(&_mutex);

  d->newTasks.clear();

  // don't call submitPreviewTask() directly as it will attempt to lock mutex again

  KLFLatexPreviewThreadPrivate::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = previewSize;
  t.largePreviewSize = largePreviewSize;

  return d->submitTask(t);
}

KLFRefPtr<KLFLatexPreviewThread::QueuedTask>
/* */  KLFLatexPreviewThread::clearAndSubmitPreviewTask(const KLFBackend::klfInput& input,
							const KLFBackend::klfSettings& settings,
							KLFLatexPreviewHandler * outputhandler)
{
  QMutexLocker mutexlocker(&_mutex);
  d->newTasks.clear();

  // don't call submitPreviewTask() directly as it will attempt to lock mutex again

  KLFLatexPreviewThreadPrivate::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = d->previewSize;
  t.largePreviewSize = d->largePreviewSize;

  return d->submitTask(t);
}

void KLFLatexPreviewThread::clearPendingTasks()
{
  QMutexLocker mutexlocker(&_mutex);
  d->newTasks.clear();
}

bool KLFLatexPreviewThread::cancelTask(KLFRefPtr<QueuedTask> task)
{
  KLF_ASSERT_CONDITION(!task->isfinished, "Cannot cancel a finished task!", return false; ) ;

  QMutexLocker mutexlocker(&_mutex);

  // find this task in list of queued tasks
  for (QQueue<KLFLatexPreviewThreadPrivate::Task>::iterator it = d->newTasks.begin();
       it != d->newTasks.end(); ++it) {
    if ((*it).qtask == task) {
      // found it. remove it from list and return.
      d->newTasks.erase(it);
      return true;
    }
  }

  return false;
}


bool KLFLatexPreviewThread::taskIsNew(KLFRefPtr<QueuedTask> task)
{
  // find this task in list of queued tasks
  for (QQueue<KLFLatexPreviewThreadPrivate::Task>::iterator it = d->newTasks.begin();
       it != d->newTasks.end(); ++it) {
    if ((*it).qtask == task) {
      return true;
    }
  }
  return false;
}
bool KLFLatexPreviewThread::taskIsRunning(KLFRefPtr<QueuedTask> task)
{
  return task->isrunning;
}
bool KLFLatexPreviewThread::taskIsFinished(KLFRefPtr<QueuedTask> task)
{
  return task->isfinished;
}

void KLFLatexPreviewThread::setPreviewSize(const QSize& previewSize)
{
  QMutexLocker mutexlocker(&_mutex);
  d->previewSize = previewSize;
}
void KLFLatexPreviewThread::setLargePreviewSize(const QSize& largePreviewSize)
{
  QMutexLocker mutexlocker(&_mutex);
  d->largePreviewSize = largePreviewSize;
}








// ------------


KLFContLatexPreview::KLFContLatexPreview(KLFLatexPreviewThread *thread)
  : QObject(thread)
{
  KLF_INIT_PRIVATE(KLFContLatexPreview) ;

  // we need to register meta-type KLFBackend::klfOutput for our signal, so register it if not yet done
  if (QMetaType::type("KLFBackend::klfOutput") == 0) {
    qRegisterMetaType<KLFBackend::klfOutput>("KLFBackend::klfOutput") ;
  }

  setThread(thread);
}

KLFContLatexPreview::~KLFContLatexPreview()
{
  KLF_DELETE_PRIVATE ;
}

KLF_DEFINE_PROPERTY_GET(KLFContLatexPreview, QSize, previewSize) ;

KLF_DEFINE_PROPERTY_GET(KLFContLatexPreview, QSize, largePreviewSize) ;


void KLFContLatexPreview::setThread(KLFLatexPreviewThread * thread)
{
  d->thread = thread;
}

bool KLFContLatexPreview::setInput(const KLFBackend::klfInput& input)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (d->input == input)
    return false;

  d->input = input;
  d->refreshPreview();
  return true;
}
bool KLFContLatexPreview::setSettings(const KLFBackend::klfSettings& settings, bool disableExtraFormats)
{
  KLFBackend::klfSettings s = settings;
  if (disableExtraFormats) {
    s.wantRaw = false;
    s.wantPDF = false;
    s.wantSVG = false;
  }

  if (d->settings == s)
    return false;

  d->settings = s;
  d->refreshPreview();
  return true;
}

bool KLFContLatexPreview::setPreviewSize(const QSize& previewSize)
{
  if (d->previewSize == previewSize)
    return false;
  d->previewSize = previewSize;
  d->refreshPreview();
  return true;
}
bool KLFContLatexPreview::setLargePreviewSize(const QSize& largePreviewSize)
{
  if (d->largePreviewSize == largePreviewSize)
    return false;
  d->largePreviewSize = largePreviewSize;
  d->refreshPreview();
  return true;
}



