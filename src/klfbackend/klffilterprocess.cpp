/***************************************************************************
 *   file klffilterprocess.cpp
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

#include <QString>
#include <QFile>
#include <QProcess>

#include <klfdefs.h>

#include "klfbackend.h"
#include "klfblockprocess.h"
#include "klffilterprocess.h"

#include "klffilterprocess_p.h"

// -----------------

// Utility function
static QString progErrorMsg(QString progname, int exitstatus, QString stderrstr, QString stdoutstr)
{
  QString stdouthtml = stdoutstr;
  QString stderrhtml = stderrstr;
  stdouthtml.replace("&", "&amp;");
  stdouthtml.replace("<", "&lt;");
  stdouthtml.replace(">", "&gt;");
  stderrhtml.replace("&", "&amp;");
  stderrhtml.replace("<", "&lt;");
  stderrhtml.replace(">", "&gt;");

  if (stderrstr.isEmpty() && stdoutstr.isEmpty())
    return QObject::tr("<p><b>%1</b> reported an error (exit status %2). No Output was generated.</p>",
		       "KLFBackend")
	.arg(progname).arg(exitstatus);
  if (stderrstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stdout output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stdouthtml);
  if (stdoutstr.isEmpty())
    return
      QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		  "<pre>\n%3</pre>", "KLFBackend")
      .arg(progname).arg(exitstatus).arg(stderrhtml);
  
  return QObject::tr("<p><b>%1</b> reported an error (exit status %2). Here is full stderr output:</p>\n"
		     "<pre>\n%3</pre><p>And here is full stdout output:</p><pre>\n%4</pre>", "KLFBackend")
    .arg(progname).arg(exitstatus).arg(stderrhtml).arg(stdouthtml);
}






// ------------------

KLFFilterProcessBlockProcess::KLFFilterProcessBlockProcess(KLFFilterProcess * fproc)
  : KLFBlockProcess(), pFproc(fproc)
{
}
KLFFilterProcessBlockProcess::~KLFFilterProcessBlockProcess()
{
}
QString KLFFilterProcessBlockProcess::getInterpreterPath(const QString& ext)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("ext = " << ext) ;
  QMap<QString,QString> interpreters = pFproc->interpreters();
  QMap<QString,QString>::Iterator it = interpreters.find(ext);
  if (it != interpreters.end()) {
    return *it;
  }
  return KLFBlockProcess::getInterpreterPath(ext);
}


// -----------------


struct KLFFilterProcessPrivate {
  KLF_PRIVATE_HEAD(KLFFilterProcess)
  {
  }

  void init(const QString& pTitle, const KLFBackend::klfSettings *settings,
            const QString& rundir, bool inheritProcessEnvironment);

  QString progTitle;
  QString programCwd;
  QStringList execEnviron;

  QStringList argv;

  QMap<QString,QString> interpreters;

  bool outputStdout;
  bool outputStderr;

  QByteArray *collectStdout;
  QByteArray *collectStderr;

  bool processAppEvents;

  // these fields are set after calling run()
  int exitStatus;
  int exitCode;

  int res;
  QString resErrorString;
};

// ---------


KLFFilterProcess::KLFFilterProcess(const QString& pTitle, const KLFBackend::klfSettings *settings,
                                   const QString& rundir)
{
  KLF_INIT_PRIVATE(KLFFilterProcess) ;
  d->init(pTitle, settings, rundir, false);
}

KLFFilterProcess::KLFFilterProcess(const QString& pTitle, const KLFBackend::klfSettings *settings,
                                   const QString& rundir, bool inheritProcessEnvironment)
{
  KLF_INIT_PRIVATE(KLFFilterProcess) ;
  d->init(pTitle, settings, rundir, inheritProcessEnvironment);
}

void KLFFilterProcessPrivate::init(const QString& pTitle, const KLFBackend::klfSettings *settings,
                                   const QString& rundir, bool inheritProcessEnvironment)
{
  progTitle = pTitle;

  collectStdout = NULL;
  collectStderr = NULL;

  outputStdout = true;
  outputStderr = false;

  interpreters = QMap<QString,QString>();

  if (rundir.size()) {
    programCwd = rundir;
  }
  if (settings != NULL) {
    if (!rundir.size()) {
      programCwd = settings->tempdir;
      klfDbg("set programCwd to : "<<programCwd) ;
    }
    QStringList curenv;
    if (inheritProcessEnvironment) {
      curenv = klfCurrentEnvironment();
    }
    execEnviron = klfMergeEnvironment(curenv, settings->execenv,
				      QStringList()<<"PATH"<<"TEXINPUTS"<<"BIBINPUTS"<<"PYTHONPATH",
                                      KlfEnvPathPrepend|KlfEnvMergeExpandVars);
    klfDbg("set execution environment to : "<<execEnviron) ;
    
    interpreters = settings->userScriptInterpreters;
  }

  processAppEvents = true;

  exitStatus = -1;
  exitCode = -1;
  res = -1;
  resErrorString = QString();
}


KLFFilterProcess::~KLFFilterProcess()
{
  KLF_DELETE_PRIVATE ;
}



QString KLFFilterProcess::progTitle() const
{
  return d->progTitle;
}
void KLFFilterProcess::setProgTitle(const QString& title)
{
  d->progTitle = title;
}

QString KLFFilterProcess::programCwd() const
{
  return d->programCwd;
}
void KLFFilterProcess::setProgramCwd(const QString& cwd)
{
  d->programCwd = cwd;
}

QStringList KLFFilterProcess::execEnviron() const
{
  return d->execEnviron;
}
void KLFFilterProcess::setExecEnviron(const QStringList& env)
{
  d->execEnviron = env;
  klfDbg("set exec environment: " << d->execEnviron);
}
void KLFFilterProcess::addExecEnviron(const QStringList& env)
{
  klfMergeEnvironment(& d->execEnviron, env);
  klfDbg("merged exec environment: " << d->execEnviron);
}

QStringList KLFFilterProcess::argv() const
{
  return d->argv;
}
void KLFFilterProcess::setArgv(const QStringList& argv)
{
  d->argv = argv;
}
void KLFFilterProcess::addArgv(const QStringList& argv)
{
  d->argv << argv;
}
void KLFFilterProcess::addArgv(const QString& argv)
{
  d->argv << argv;
}

bool KLFFilterProcess::outputStdout() const
{
  return d->outputStdout;
}
void KLFFilterProcess::setOutputStdout(bool on)
{
  d->outputStdout = on;
}

bool KLFFilterProcess::outputStderr() const
{
  return d->outputStderr;
}
void KLFFilterProcess::setOutputStderr(bool on)
{
  d->outputStderr = on;
}

void KLFFilterProcess::collectStdoutTo(QByteArray * stdoutstore)
{
  setOutputStdout(true);
  d->collectStdout = stdoutstore;
}
void KLFFilterProcess::collectStderrTo(QByteArray * stderrstore)
{
  setOutputStderr(true);
  d->collectStderr = stderrstore;
}

bool KLFFilterProcess::processAppEvents()
{
  return d->processAppEvents;
}

void KLFFilterProcess::setProcessAppEvents(bool on)
{
  d->processAppEvents = on;
}

int KLFFilterProcess::exitStatus() const
{
  return d->exitStatus;
}
int KLFFilterProcess::exitCode() const
{
  return d->exitCode;
}

int KLFFilterProcess::resultStatus() const
{
  return d->res;
}
QString KLFFilterProcess::resultErrorString() const
{
  return d->resErrorString;
}

QMap<QString,QString> KLFFilterProcess::interpreters() const
{
  return d->interpreters;
}

bool KLFFilterProcess::do_run(const QByteArray& indata, const QMap<QString, QByteArray*> outdatalist)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFFilterProcessBlockProcess proc(this);

  d->exitCode = 0;
  d->exitStatus = 0;

  KLF_ASSERT_CONDITION(d->argv.size() > 0, "argv array is empty! No program is given!", return false; ) ;
  
  proc.setWorkingDirectory(d->programCwd);

  proc.setProcessAppEvents(d->processAppEvents);

  klfDbg("about to exec "<<d->progTitle<<" ...") ;
  klfDbg("\t"<<qPrintable(d->argv.join(" "))) ;
  bool r = proc.startProcess(d->argv, indata, d->execEnviron);
  klfDbg(d->progTitle<<" returned.") ;

  if (!r) {
    klfDbg("couldn't launch " << d->progTitle) ;
    d->res = KLFFP_NOSTART;
    d->resErrorString = QObject::tr("Unable to start %1 program `%2'!", "KLFBackend").arg(d->progTitle, d->argv[0]);
    return false;
  }
  if (!proc.processNormalExit()) {
    klfDbg(d->progTitle<<" did not exit normally (crashed)") ;
    d->exitStatus = proc.exitStatus();
    d->exitCode = -1;
    d->res = KLFFP_NOEXIT;
    d->resErrorString = QObject::tr("Program %1 crashed!", "KLFBackend").arg(d->progTitle);
    return false;
  }
  if (proc.processExitStatus() != 0) {
    d->exitStatus = 0;
    d->exitCode = proc.processExitStatus();
    klfDbg(d->progTitle<<" exited with code "<<d->exitCode) ;
    d->res = KLFFP_NOSUCCESSEXIT;
    d->resErrorString = progErrorMsg(d->progTitle, proc.processExitStatus(), proc.readStderrString(),
				     proc.readStdoutString());
    return false;
  }

  if (d->collectStdout != NULL) {
    *d->collectStdout = proc.getAllStdout();
  }
  if (d->collectStderr != NULL) {
    *d->collectStderr = proc.getAllStderr();
  }

  for (QMap<QString,QByteArray*>::const_iterator it = outdatalist.begin(); it != outdatalist.end(); ++it) {
    QString outFileName = it.key();
    QByteArray * outdata = it.value();
      
    KLF_ASSERT_NOT_NULL(outdata, "Given NULL outdata pointer for file "<<outFileName<<" !", return false; ) ;

    klfDbg("Will collect output in file "<<(outFileName.isEmpty()?QString("(stdout)"):outFileName)
	   <<" to its corresponding QByteArray pointer="<<outdata) ;

    if (outFileName.isEmpty()) {
      // empty outFileName means to use standard output
      *outdata = QByteArray();
      if (d->outputStdout) {
	QByteArray stdoutdata = (d->collectStdout != NULL) ? *d->collectStdout : proc.getAllStdout();
	*outdata += stdoutdata;
      }
      if (d->outputStderr) {
	QByteArray stderrdata = (d->collectStderr != NULL) ? *d->collectStderr : proc.getAllStderr();
	*outdata += stderrdata;
      }
      if (outdata->isEmpty()) {
	// no data
	QString stderrstr = (!d->outputStderr) ? ("\n"+proc.readStderrString()) : QString();
	klfDbg(d->progTitle<<" did not provide any data. Error message: "<<stderrstr);
	d->res = KLFFP_NODATA;
	d->resErrorString = QObject::tr("Program %1 did not provide any output data.", "KLFBackend")
	  .arg(d->progTitle) + stderrstr;
	return false;
      }
      // read standard output to buffer, continue with other possible outputs
      continue;
    }

    if (!QFile::exists(outFileName)) {
      klfDbg("File "<<outFileName<<" did not appear after running "<<d->progTitle) ;
      d->res = KLFFP_NODATA;
      d->resErrorString = QObject::tr("Output file didn't appear after having called %1!", "KLFBackend")
	.arg(d->progTitle);
      return false;
    }

    // read output file into outdata
    QFile outfile(outFileName);
    r = outfile.open(QIODevice::ReadOnly);
    if ( ! r ) {
      klfDbg("File "<<outFileName<<" cannot be read (after running "<<d->progTitle<<")") ;
      d->res = KLFFP_DATAREADFAIL;
      d->resErrorString = QObject::tr("Can't read file '%1'!\n", "KLFBackend").arg(outFileName);
      return false;
    }
      
    *outdata = outfile.readAll();
    klfDbg("Read file "<<outFileName<<", got data, length="<<outdata->size());
  }

  klfDbg(d->progTitle<<" was successfully run and output successfully retrieved.") ;

  // all OK
  d->exitStatus = 0;
  d->exitCode = 0;
  d->res = KLFFP_NOERR;
  d->resErrorString = QString();

  return true;
}


QByteArray KLFFilterProcess::collectedStdout() const
{
  if (!d->outputStdout || d->collectStdout == NULL) {
    return QByteArray();
  }
  return *d->collectStdout;
}
QByteArray KLFFilterProcess::collectedStderr() const
{
  if (!d->outputStderr || d->collectStderr == NULL) {
    return QByteArray();
  }
  return *d->collectStderr;
}
