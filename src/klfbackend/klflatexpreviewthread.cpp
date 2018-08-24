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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE( KLFLatexPreviewThread ) ;

  // we need to register meta-type KLFBackend::klfOutput/klfInput/klfSettings for our
  // signal/meta-object call, so register it if not yet done
  if (QMetaType::type("KLFBackend::klfOutput") == 0) {
    qRegisterMetaType<KLFBackend::klfOutput>("KLFBackend::klfOutput") ;
  }
  if (QMetaType::type("KLFBackend::klfInput") == 0) {
    qRegisterMetaType<KLFBackend::klfInput>("KLFBackend::klfInput") ;
  }
  if (QMetaType::type("KLFBackend::klfSettings") == 0) {
    qRegisterMetaType<KLFBackend::klfSettings>("KLFBackend::klfSettings") ;
  }
  if (QMetaType::type("KLFLatexPreviewThreadWorker::Task") == 0) {
    qRegisterMetaType<KLFLatexPreviewThreadWorker::Task>("KLFLatexPreviewThreadWorker::Task") ;
  }
  if (QMetaType::type("KLFLatexPreviewThread::TaskId") == 0) {
    qRegisterMetaType<KLFLatexPreviewThread::TaskId>("KLFLatexPreviewThread::TaskId") ;
  }


  //
  // create a worker that will do all the job for us
  //

  d->worker = new KLFLatexPreviewThreadWorker;
  d->worker->moveToThread(this);

  // create a direct-connection abort signal; this is fine because worker.abort() is thread-safe.
  connect(d, SIGNAL(internalRequestAbort()), d->worker, SLOT(abort()), Qt::DirectConnection);
  
  // connect the signal that submits a new job.
  connect(d, SIGNAL(internalRequestSubmitNewTask(KLFLatexPreviewThreadWorker::Task, bool,
						 KLFLatexPreviewThread::TaskId)),
	  d->worker, SLOT(threadSubmitTask(KLFLatexPreviewThreadWorker::Task, bool,
                                           KLFLatexPreviewThread::TaskId)),
	  Qt::QueuedConnection);
  // signal to clear all pending jobs
  connect(d, SIGNAL(internalRequestClearPendingTasks()), d->worker, SLOT(threadClearPendingTasks()),
	  Qt::QueuedConnection);
  // signal to cancel a specific task
  connect(d, SIGNAL(internalRequestCancelTask(KLFLatexPreviewThread::TaskId)),
	  d->worker, SLOT(threadCancelTask(KLFLatexPreviewThread::TaskId)),
	  Qt::QueuedConnection);
}

KLFLatexPreviewThread::~KLFLatexPreviewThread()
{
  stop();

  if (d->worker) {
    delete d->worker;
  }

  KLF_DELETE_PRIVATE ;
}


QSize KLFLatexPreviewThread::previewSize() const
{ return d->previewSize; }
QSize KLFLatexPreviewThread::largePreviewSize() const
{ return d->largePreviewSize; }

void KLFLatexPreviewThread::setPreviewSize(const QSize& previewSize)
{ d->previewSize = previewSize; }
void KLFLatexPreviewThread::setLargePreviewSize(const QSize& largePreviewSize)
{ d->largePreviewSize = largePreviewSize; }


void KLFLatexPreviewThread::start(Priority priority)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // fire up the thread
  QThread::start(priority);
}

void KLFLatexPreviewThread::stop()
{
  // tell thread to stop, and wait for it
  emit d->internalRequestAbort();
  quit();
  wait();
}


void KLFLatexPreviewThread::run()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // fire up and enter the main loop.
  QThread::run();
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::submitPreviewTask(const KLFBackend::klfInput& input,
						const KLFBackend::klfSettings& settings,
						KLFLatexPreviewHandler * outputhandler,
						const QSize& previewSize,
						const QSize& largePreviewSize)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = previewSize;
  t.largePreviewSize = largePreviewSize;

  return d->submitTask(t, false, -1);
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::submitPreviewTask(const KLFBackend::klfInput& input,
						const KLFBackend::klfSettings& settings,
						KLFLatexPreviewHandler * outputhandler)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = d->previewSize;
  t.largePreviewSize = d->largePreviewSize;

  return d->submitTask(t, false, -1);
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::clearAndSubmitPreviewTask(const KLFBackend::klfInput& input,
							const KLFBackend::klfSettings& settings,
							KLFLatexPreviewHandler * outputhandler,
							const QSize& previewSize,
							const QSize& largePreviewSize)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = previewSize;
  t.largePreviewSize = largePreviewSize;

  return d->submitTask(t, true, -1);
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::clearAndSubmitPreviewTask(const KLFBackend::klfInput& input,
							const KLFBackend::klfSettings& settings,
							KLFLatexPreviewHandler * outputhandler)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = d->previewSize;
  t.largePreviewSize = d->largePreviewSize;

  return d->submitTask(t, true, -1);
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::replaceSubmitPreviewTask(KLFLatexPreviewThread::TaskId replaceId,
						       const KLFBackend::klfInput& input,
						       const KLFBackend::klfSettings& settings,
						       KLFLatexPreviewHandler * outputhandler,
						       const QSize& previewSize,
						       const QSize& largePreviewSize)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = previewSize;
  t.largePreviewSize = largePreviewSize;

  return d->submitTask(t, false, replaceId);
}

KLFLatexPreviewThread::TaskId
/* */  KLFLatexPreviewThread::replaceSubmitPreviewTask(KLFLatexPreviewThread::TaskId replaceId,
						       const KLFBackend::klfInput& input,
						       const KLFBackend::klfSettings& settings,
						       KLFLatexPreviewHandler * outputhandler)
{
  KLFLatexPreviewThreadWorker::Task t;
  t.input = input;
  t.settings = settings;
  t.handler = outputhandler;
  t.previewSize = d->previewSize;
  t.largePreviewSize = d->largePreviewSize;

  return d->submitTask(t, false, replaceId);
}



void KLFLatexPreviewThread::cancelTask(TaskId task)
{
  emit d->internalRequestCancelTask(task);
}
void KLFLatexPreviewThread::clearPendingTasks()
{
  emit d->internalRequestClearPendingTasks();
}




// -----



void KLFLatexPreviewThreadWorker::threadSubmitTask(Task task, bool clearOtherJobs, TaskId replaceTaskId)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (clearOtherJobs) {
    threadClearPendingTasks();
  }
  if (replaceTaskId) {
    threadCancelTask(replaceTaskId);
  }

  // enqueue the new task
  newTasks.enqueue(task);

  klfDbg("enqueued task id="<<task.taskid) ;

  // and notify ourself in the event loop that there are more jobs to process
  QMetaObject::invokeMethod(this, "threadProcessJobs", Qt::QueuedConnection);
}

bool KLFLatexPreviewThreadWorker::threadCancelTask(TaskId taskid)
{
  int k;
  for (k = 0; k < newTasks.size(); ++k) {
    if (newTasks.at(k).taskid == taskid) {
      newTasks.removeAt(k);
      return true;
    }
  }

  // this might not be an error, it could be that the task completed before we had
  // a chance to cancel it
  klfDbg("No such task ID: "<<taskid) ;
  return false;
}

void KLFLatexPreviewThreadWorker::threadClearPendingTasks()
{
  newTasks.clear();
}

void KLFLatexPreviewThreadWorker::threadProcessJobs()
{
  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

  Task task;
  KLFBackend::klfOutput ouroutput;

  if (!newTasks.size()) {
    return;
  }

  if (_abort) {
    return;
  }

  // fetch task info
  task = newTasks.dequeue();

  klfDbg("processing job ID="<<task.taskid) ;

  QImage img, prev, lprev;
  if ( task.input.latex.trimmed().isEmpty() ) {
    QMetaObject::invokeMethod(task.handler, "latexPreviewReset", Qt::QueuedConnection);
  } else {
    // and GO!
    klfDbg("worker: running KLFBackend::getLatexFormula()") ;
    ouroutput = KLFBackend::getLatexFormula(task.input, task.settings, false);
    img = ouroutput.result;

    klfDbg("got result: status="<<ouroutput.status) ;

    if (ouroutput.status != 0) {
      // error...
      QMetaObject::invokeMethod(task.handler, "latexPreviewError", Qt::QueuedConnection,
				Q_ARG(QString, ouroutput.errorstr),
				Q_ARG(int, ouroutput.status));
    } else {
      // this method must be called first (by API design)
      QMetaObject::invokeMethod(task.handler, "latexOutputAvailable", Qt::QueuedConnection,
				Q_ARG(KLFBackend::klfOutput, ouroutput));
      if (task.previewSize.isValid()) {
	prev = img;
	if (prev.width() > task.previewSize.width() || prev.height() > task.previewSize.height()) {
	  prev = img.scaled(task.previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
      }
      if (task.largePreviewSize.isValid()) {
	lprev = img;
	if (lprev.width() > task.largePreviewSize.width() || lprev.height() > task.largePreviewSize.height()) {
	  lprev = img.scaled(task.largePreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
      }
      
      QMetaObject::invokeMethod(task.handler, "latexPreviewAvailable", Qt::QueuedConnection,
				Q_ARG(QImage, prev),
				Q_ARG(QImage, lprev),
				Q_ARG(QImage, img));
      if (task.previewSize.isValid()) {
	QMetaObject::invokeMethod(task.handler, "latexPreviewImageAvailable", Qt::QueuedConnection,
				  Q_ARG(QImage, prev));
      }
      if (task.largePreviewSize.isValid()) {
	QMetaObject::invokeMethod(task.handler, "latexPreviewLargeImageAvailable", Qt::QueuedConnection,
				  Q_ARG(QImage, lprev));
      }
      QMetaObject::invokeMethod(task.handler, "latexPreviewFullImageAvailable", Qt::QueuedConnection,
				Q_ARG(QImage, img));
    }
  }

  klfDbg("about to invoke delayed threadProcessJobs.") ;

  // continue processing jobs, but let the event loop have a chance to run a bit too.
  QMetaObject::invokeMethod(this, "threadProcessJobs", Qt::QueuedConnection);

  klfDbg("threadProcessJobs: end") ;
}







// ------------


KLFContLatexPreview::KLFContLatexPreview(KLFLatexPreviewThread *thread)
  : QObject(thread)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLF_INIT_PRIVATE( KLFContLatexPreview ) ;

  setThread(thread);
}

KLFContLatexPreview::~KLFContLatexPreview()
{
  KLF_DELETE_PRIVATE ;
}

KLF_DEFINE_PROPERTY_GET(KLFContLatexPreview, QSize, previewSize) ;

KLF_DEFINE_PROPERTY_GET(KLFContLatexPreview, QSize, largePreviewSize) ;



bool KLFContLatexPreview::enabled() const
{
  return d->enabled;
}

void KLFContLatexPreview::setEnabled(bool enabled)
{
  d->enabled = enabled;
}


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



