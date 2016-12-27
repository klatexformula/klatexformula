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

#include <QProcess>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>

#include <klfutil.h>
#include "klfblockprocess.h"

static bool is_binary_file(QString fn)
{
  klfDbg("is_binary_file("<<fn<<")") ;
  if (!QFile::exists(fn)) {
    fn = klfSearchPath(fn);
    klfDbg("is_binary_file: file doesn't exist directly, path search gave "<<fn) ;
  }
  QFile fpeek(fn);
  if (!fpeek.open(QIODevice::ReadOnly)) {
    klfDbg("fn="<<fn<<", Can't peek into file "<<fn<<"!") ;
    return true; // assumption by default
  }
  QByteArray line;
  int n = 0, j;
  while (n++ < 5 && (line = fpeek.readLine(1024)).size()) {
    for (j = 0; j < line.size(); ++j) {
      if ((int)line[j] >= 127 || (int)line[j] <= 0) {
        klfDbg("fn="<<fn<<" found binary char '"<<(char)line[j]<<"'=="<<(int)line[j]
               <<" on line "<<n<<", column "<< j) ;
        return true;
      }
    }
  }
  klfDbg("fn="<<fn<<", file seems to be ascii based on the first few lines") ;
  return false;
}


#if defined(Q_OS_WIN32)
const static QString script_extra_paths = QString("C:\\Python27");
static QByteArray get_script_process_type(const QString& name)
{
  // e.g. KLF_PYTHON_EXECUTABLE
  QString envname = QString("KLF_%1_EXECUTABLE").arg(name.toUpper());
  QByteArray path = qgetenv(envname.toLatin1().constData());
  if (path.size() > 0)
    return path;
  // try to find the executable somewhere on the system
  // allow suffixes to the executables, e.g. for python27
  path = klfSearchPath(name+"*.exe", script_extra_paths).toLocal8Bit();
  if (path.size() > 0)
    return path;
  return QByteArray();
}
static QByteArray get_script_process(QString fn)
{
  fn = fn.toLower();
  if (fn.endsWith(".py")) {
    return get_script_process_type(QLatin1String("python"));
  }
  if (fn.endsWith(".rb")) {
    return get_script_process_type(QLatin1String("ruby"));
  }
  if (fn.endsWith(".sh")) {
    return get_script_process_type(QLatin1String("bash"));
  }
  if (fn.endsWith(".pl")) {
    return get_script_process_type(QLatin1String("perl"));
  }
  return QByteArray();
}
#endif


KLFBlockProcess::KLFBlockProcess(QObject *p)  : QProcess(p)
{
  mProcessAppEvents = true;
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ourProcExited()));
}


KLFBlockProcess::~KLFBlockProcess()
{
}

void KLFBlockProcess::ourProcGotOurStdinData()
{
}

void KLFBlockProcess::ourProcExited()
{
  _runstatus = 1; // exited
}
bool KLFBlockProcess::startProcess(QStringList cmd, QStringList env)
{
  return startProcess(cmd, QByteArray(), env);
}

bool KLFBlockProcess::startProcess(QStringList cmd, QByteArray stdindata, QStringList env)
{
  klfDbg("Running: "<<cmd<<", stdindata/size="<<stdindata.size());

  _runstatus = 0;

  KLF_ASSERT_CONDITION(cmd.size(), "Empty command list given.", return false;) ;

#if defined(Q_OS_UNIX)

  // for epstopdf bug in ubuntu: peek into executable, see if it is script. if it is, run with 'sh' on *nix's.
  // this is a weird bug with QProcess that will not execute some script files like epstopdf.

  if (!is_binary_file(cmd[0])) {
    // explicitly add a wrapper ('sh' only works for bash shell scripts, so use 'env') (we're on *nix, so OK)
    cmd.prepend("/usr/bin/env");
    klfDbg("target "<<cmd[0]<<" is not binary, prepending /usr/bin/env to command. cmd = " <<cmd) ;
  }

#endif

#if defined(Q_OS_WIN32)

  if (!is_binary_file(cmd[0])) {
    // check what script type it is, and try to use pre-defined executables in shell variables (HACK!!)
    /// FIXME: This is ugly! Need better solution!
    QByteArray exec_proc = get_script_process(cmd[0]);
    if (exec_proc.size()) {
      cmd.prepend(exec_proc);
    }
  }

#endif

  QString program = cmd[0];

  klfDbg("Running cmd="<<cmd);
  klfDbg("env="<<env<<", curenv="<<environment());

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

  if (mProcessAppEvents) {
    klfDbg("letting app process events ...") ;
    while (_runstatus == 0) {
      qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 1000);
      klfDbg("events processed, maybe more?") ;
    }
  } else {
    if (!waitForFinished()) {
      klfDbg("Can't wait for finished!");
      return false;
    }
  }
  klfDbg("Process should have finished now.");

  if (_runstatus < 0) { // some error occurred somewhere
    klfDbg("some error occurred, _runstatus="<<_runstatus) ;
    return false;
  }

  return true;
}

