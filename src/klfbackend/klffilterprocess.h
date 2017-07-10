/***************************************************************************
 *   file klffilterprocess.h
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


#ifndef KLFFILTERPROCESS_H
#define KLFFILTERPROCESS_H


#include <klfdefs.h>
#include <klfblockprocess.h>
#include <klfbackend.h>


#define KLFFP_NOERR 0
#define KLFFP_NOSTART 1
#define KLFFP_NOEXIT 2
#define KLFFP_NOSUCCESSEXIT 3
#define KLFFP_NODATA 4
#define KLFFP_DATAREADFAIL 5
#define KLFFP_PAST_LAST_VALUE 6



struct KLFFilterProcessPrivate;
class KLFFilterProcessBlockProcess;

class KLF_EXPORT KLFFilterProcess
{
public:
  KLFFilterProcess(const QString& pTitle = QString(), const KLFBackend::klfSettings *settings = NULL,
                   const QString& rundir = QString());
  KLFFilterProcess(const QString& pTitle, const KLFBackend::klfSettings *settings,
                   const QString& rundir, bool inheritProcessEnvironment);
  virtual ~KLFFilterProcess();


  QString progTitle() const;
  void setProgTitle(const QString& title);

  QString programCwd() const;
  void setProgramCwd(const QString& cwd);

  QStringList execEnviron() const;
  void setExecEnviron(const QStringList& env);
  void addExecEnviron(const QStringList& env);

  QStringList argv() const;
  void setArgv(const QStringList& argv);
  void addArgv(const QStringList& argv);
  void addArgv(const QString& argv);

  bool outputStdout() const;
  /** Set this to false to ignore output on stdout of the program. */
  void setOutputStdout(bool on);

  bool outputStderr() const;
  /** Set this to true to also read stderr as part of the output. If false (the default), stderr
   * output is only reported in the error message in case nothing came out on stdout. */
  void setOutputStderr(bool on);

  /** Set a QByteArray where all stdout data will be stored */
  void collectStdoutTo(QByteArray * stdoutstore);
  /** Set a QByteArray where all stderr data will be stored */
  void collectStderrTo(QByteArray * stderrstore);

  /** See \ref setProcessAppEvents() */
  bool processAppEvents();
  /** specify whether or not to call regularly qApp->processEvents() while executing.
   * This will prevent the GUI to freeze. Enabled is the default. However you can choose to
   * disable this behavior by passing FALSE here, e.g. if you're not in the GUI thread. */
  void setProcessAppEvents(bool processEvents);


  /** After run(), this is set to the exit status of the process. See QProcess::exitStatus() */
  virtual int exitStatus() const;
  /** After run(), this is set to the exit code of the process. See QProcess::exitCode() */
  virtual int exitCode() const;

  /** This is one of the KLFFP_* define's, such as \ref KLFFP_NOSTART, or \ref KLFFP_NOERR if all OK. */
  virtual int resultStatus() const;
  /** An explicit error string in case the resultStatus() indicated an error. */
  virtual QString resultErrorString() const;


  bool run(const QString& outFileName, QByteArray *outdata)
  {
    return run(QByteArray(), outFileName, outdata);
  }

  bool run(const QByteArray& indata, const QString& outFileName, QByteArray *outdata)
  {
    QMap<QString,QByteArray*> fout; fout[outFileName] = outdata;
    return do_run(indata, fout);
  }

  bool run(const QMap<QString, QByteArray*> outdata)
  {
    return do_run(QByteArray(), outdata);
  }

  bool run(const QByteArray& indata = QByteArray())
  {
    return do_run(indata, QMap<QString, QByteArray*>());
  }

  /**
   *
   * \note multiple output files possible. Each data is retreived into a pointer given by outdata map.
   *
   * \param indata a QByteArray to write into the program's standard input
   * \param outdatalist a QMap with keys being files that are created by the program. These files are
   *   read and their contents stored in the QByteArray's pointed by the corresponding pointer.
   * \param resError the klfOutput object is initialized to the corresponding error if an error occurred.
   *
   * An empty file name in the list means to collect the standard output.
   *
   * \returns TRUE/FALSE for success/failure, respectively.
   */
  bool run(const QByteArray& indata, const QMap<QString, QByteArray*> outdatalist)
  {
    return do_run(indata, outdatalist);
  }

protected:

  friend class KLFFilterProcessBlockProcess;
  virtual QMap<QString,QString> interpreters() const;

  /** \brief Actually run the process
   *
   * Each run() overload above internally just redirects to this function.
   *
   * Subclasses may reimplement if they want to do some bookkeeping, cleaning up, keeping
   * a log, etc.
   *
   */
  virtual bool do_run(const QByteArray& indata, const QMap<QString, QByteArray*> outdatalist);

  /** \brief The collected stdout data of the process that just ran.
   *
   * Convenience method for subclasses.  Stdout data collection must have been enabled
   * (with \a setOutputStdout() and \a collectStdoutTo()).
   */
  QByteArray collectedStdout() const;
  /** \brief The collected stderr data of the process that just ran.
   *
   * Convenience method for subclasses.  Stderr data collection must have been enabled
   * (with \a setOutputStderr() and \a collectStderrTo()).
   */
  QByteArray collectedStderr() const;

private:
  KLF_DECLARE_PRIVATE(KLFFilterProcess) ;
};








#endif
