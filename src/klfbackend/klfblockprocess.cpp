/***************************************************************************
 *   file klfblockprocess.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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

#include <ctype.h>

#include <qprocess.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qfileinfo.h>

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
  klfDbg("Running: "<<cmd<<", stdindata/size="<<stdindata.size());

  _runstatus = 0;

  KLF_ASSERT_CONDITION(cmd.size(), "Empty command list given.", return false;) ;

#if defined(Q_OS_UNIX) && defined(KLFBACKEND_QT4)

  // ** epstopdf bug in ubuntu: peek into executable, see if it is script. if it is, run with 'sh' on *nix's.
  // this is a weird bug with QProcess that will not execute some script files like epstopdf.

  {
    QString fn = cmd[0];
    if (!QFile::exists(fn))
      fn = klfSearchPath(cmd[0]);
    QFile fpeek(fn);
    if (!fpeek.open(QIODevice::ReadOnly)) {
      klfDbg("cmd[0]="<<cmd[0]<<", Can't peek into file "<<fn<<"!") ;
    } else {
      QByteArray line;
      int n = 0, j;
      bool isbinary = false;
      while (n++ < 3 && (line = fpeek.readLine()).size()) {
	for (j = 0; j < line.size(); ++j) {
          if ( ! isascii(line[j]) ) {
	    isbinary = true;
	    break;
	  }
	}
	if (isbinary)
	  break;
      }
      if (!isbinary) {
	// explicitely run with /usr/bin/env (we're on *nix, so OK)
	cmd.prepend("/usr/bin/env");
      }
    }
  }
    

#endif

  QString program = cmd[0];

  klfDbg("Running cmd="<<cmd);
  klfDbg("env="<<env<<", curenv="<<environment());

#ifdef KLFBACKEND_QT4
  if (env.size() > 0) {
    setEnvironment(env);
  }

  QStringList args = cmd;
  args.erase(args.begin());
  klfDbg("Starting "<<program<<", "<<args) ;
  start(program, args);
  if ( ! waitForStarted() ) {
    klfDbg("Can't wait for started! Error="<<error()) ;
    return false;
  }

  write(stdindata.constData(), stdindata.size());
  closeWriteChannel();

  klfDbg("wrote input data (size="<<stdindata.size()<<")") ;

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
    if (!waitForFinished()) {
      klfDbg("Can't wait for finished!");
      return false;
    }
  }
  klfDbg("Process should have finished now.");
#else
  while (_runstatus == 0) {
    qApp->processEvents(QEventLoop::ExcludeUserInput);
  }
#endif

  if (_runstatus < 0) { // some error occurred somewhere
    klfDbg("some error occurred, _runstatus="<<_runstatus) ;
    return false;
  }

  return true;
}



KLF_EXPORT QStringList klf_cur_environ()
{
  QStringList curenvironment;
#ifdef KLFBACKEND_QT4
  curenvironment = QProcess::systemEnvironment();
#else
  extern char ** environ;
  int k;
  for (k = 0; environ[k] != NULL; ++k) {
    curenvironment.append(QString::fromLocal8Bit(environ[k]));
  }
#endif
  return curenvironment;
}
