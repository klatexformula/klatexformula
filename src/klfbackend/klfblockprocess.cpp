/***************************************************************************
 *   file klfblockprocess.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#include <qprocess.h>
#include <qapplication.h>
#include <qeventloop.h>

#include "klfblockprocess.h"

KLFBlockProcess::KLFBlockProcess(QObject *p)  : QProcess(p)
{
#ifdef KLFBACKEND_QT4
  mProcessAppEvents = true;
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ourProcExited()));
#else
  connect(this, SIGNAL(wroteToStdin()), this, SLOT(ourProcGotOurStdinData()));
  connect(this, SIGNAL(processExited()), this, SLOT(ourProcExited()));
#endif
}


KLFBlockProcess::~KLFBlockProcess()
{
}

void KLFBlockProcess::ourProcGotOurStdinData()
{
#ifndef KLFBACKEND_QT4
  closeStdin();
#endif
}

void KLFBlockProcess::ourProcExited()
{
  _runstatus = 1; // exited
}
#ifndef KLFBACKEND_QT4
bool KLFBlockProcess::startProcess(QStringList cmd, QCString str, QStringList env)
{
  return startProcess(cmd, QByteArray().duplicate(str.data(), str.length()), env);
}
#endif
bool KLFBlockProcess::startProcess(QStringList cmd, QStringList env)
{
  return startProcess(cmd, QByteArray(), env);
}

bool KLFBlockProcess::startProcess(QStringList cmd, QByteArray stdindata, QStringList env)
{
  _runstatus = 0;

#ifdef KLFBACKEND_QT4
  if (env.size() > 0)
    setEnvironment(env);

  QString program = cmd.front();
  QStringList args = cmd;
  args.erase(args.begin());
  start(program, args);
  if ( ! waitForStarted() )
    return false;

  write(stdindata.constData(), stdindata.size());
  closeWriteChannel();
#else
  setArguments(cmd);
  QStringList *e = &env;
  if (e->size() == 0)
    e = 0;

  if (! start(e) )
    return false;

  writeToStdin(stdindata);
  // slot ourProcGotOutStdinData() should be called, which closes input
#endif

#ifdef KLFBACKEND_QT4
  if (mProcessAppEvents) {
    while (_runstatus == 0) {
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  } else {
    waitForFinished();
  }
#else
  while (_runstatus == 0) {
    qApp->processEvents(QEventLoop::ExcludeUserInput);
  }
#endif

  if (_runstatus < 0) { // some error occurred somewhere
    return false;
  }

  return true;
}
