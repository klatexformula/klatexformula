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
#include <QThread>

#include <klfutil.h>
#include <klfsysinfo.h>
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

#if defined(Q_OS_WIN)
const static QString script_extra_paths = QString("C:\\Python27;C:\\Python*");
const static QString exe_suffix = ".exe";
#elif defined(Q_OS_MAC)
const static QString script_extra_paths =
                "/usr/bin:/bin:/usr/local/bin:/usr/sbin:/sbin:/usr/local/sbin" // general unix
                "/usr/local/opt/*/bin:/opt/local/bin:" // homebrew
                "/opt/local/sbin" // macports
                "/Library/TeX/texbin:/usr/texbin"; // mactex binaries
const static QString exe_suffix = "";
#else
const static QString script_extra_paths =
                "/usr/bin:/bin:/usr/local/bin:/usr/sbin:/sbin:/usr/local/sbin"; // general unix paths
const static QString exe_suffix = "";
#endif

/*
static QByteArray get_script_process_type(const QString& name)
{
  // e.g. KLF_PYTHON_EXECUTABLE
  QString envname = QString("KLF_%1_EXECUTABLE").arg(name.toUpper());
  QByteArray path = qgetenv(envname.toLatin1().constData());
  if (path.size() > 0)
    return path;
  // try to find the executable somewhere on the system
  // allow suffixes to the executables, e.g. for python27
  path = klfSearchPath(name+"*"+exe_suffix, script_extra_paths).toLocal8Bit();
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

  qWarning() << KLF_FUNC_NAME << "Unknown script type: " << fn << "\n";
  return get_script_process_type(QLatin1String("env"));
}
*/


// static
QString KLFBlockProcess::detectInterpreterPath(const QString& interp, const QStringList & addpaths)
{
  QString search_paths = script_extra_paths;
  search_paths += addpaths.join(KLF_PATH_SEP);
  // first, try exact name (python.exe, bash.exe)
  QString s = klfSearchPath(interp+exe_suffix, script_extra_paths);
  if (!s.isEmpty()) {
    return s;
  }
  // otherwise, try name with some suffix (e.g. python2.exe) -- dont directly try with
  // wildcard, because we want the exact name if it exists (and not some other program,
  // such as 'bashbug', which happened to be found first)
  return klfSearchPath(interp+"*"+exe_suffix, script_extra_paths);
}




KLFBlockProcess::KLFBlockProcess(QObject *p)
  : QProcess(p)
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

QString KLFBlockProcess::getInterpreterPath(const QString & ext)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("ext = " << ext) ;
  if (ext == "py") {
    return detectInterpreterPath("python");
  } else if (ext == "sh") {
    return detectInterpreterPath("bash");
  } else if (ext == "rb") {
    return detectInterpreterPath("ruby");
  }
  return QString();
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

  // For scripts, use the interpreter explicitly.  This so that the script doesn't have to
  // be executable, and also for an old bug on Ubuntu with epstopdf.
  //
  // We peek into executable to see if it is script. If it is, use the correct interpreter.

  if (!is_binary_file(cmd[0])) {
    // check what script type it is and invoke the corresponding interpreter.
    QString ext = cmd[0].split('.').last();
    QByteArray exec_proc = getInterpreterPath(ext).toLocal8Bit();
    if (exec_proc.size()) {
      cmd.prepend(exec_proc);
    }
  }

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
    klfDbg("letting current thread (="<<QThread::currentThread()<<") process events ...") ;
    while (_runstatus == 0) {
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1000);
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

