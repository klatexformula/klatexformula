/***************************************************************************
 *   file klfblockprocess.h
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

#ifndef KLFBLOCKPROCESS_H
#define KLFBLOCKPROCESS_H

//! Defines the KLFBlockProcess class
/** \file
 * Defines the KLFBlockProcess class */


#include <klfdefs.h>

#include <QProcess>
#include <QString>
#include <QByteArray>


//! A QProcess subclass for code-blocking process execution
/** A Code-blocking (but not GUI-blocking) process executor
 *
 * Use for example like:
 * \code
 *   KLFBlockProcess proc;
 *   QStringList args;
 *   args << "ls" << "/dev";
 *   proc.startProcess(args);
 *   QString alldevices = proc.readStdoutString();
 * \endcode
 *
 * This class provides functionality for passing data to STDIN and
 * getting data from STDOUT and STDERR afterwards.
 *
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFBlockProcess : public QProcess
{
  Q_OBJECT
public:
  /** Normal constructor, like QProcess constructor */
  KLFBlockProcess(QObject *parent = 0);
  /** Normal destructor */
  ~KLFBlockProcess();

  /** specify whether or not to call regularly qApp->processEvents() while executing.
   * This will prevent the GUI to freeze. Enabled is the default. However you can choose to
   * disable this behavior by passing FALSE here. */
  inline void setProcessAppEvents(bool processAppEvents) { mProcessAppEvents = processAppEvents; }

  /** Returns all standard error output as a QByteArray. This function is to standardize the
   * readStderr() and readAllStandardError() functions in QT 3 or QT 4 respectively */
  QByteArray getAllStderr() {
    return readAllStandardError();
  }

  /** Returns all standard output as a QByteArray. This function is to standardize the
   * readStdout() and readAllStandardOutput() functions in QT 3 or QT 4 respectively */
  QByteArray getAllStdout() {
    return readAllStandardOutput();
  }

  /** Same as QProcess::exitStatus()==NormalExit */
  bool processNormalExit() const {
    return QProcess::exitStatus() == NormalExit;
  }

  /** Same as QProcess::exitCode() */
  int processExitStatus() const {
    return exitCode();
  }

  // detect python, shell, etc. interpreters using some standard paths
  static QString detectInterpreterPath(const QString& interp,
                                       const QStringList & addpaths = QStringList());


  /** \brief The interpter path to use for the given extension.
   *
   * This function will be queried by \ref startProcess() when we have to execute a
   * script.
   *
   * Subclasses may reimplement to e.g. query user settings etc. Subclasses may of course
   * also make use of \ref detectInterpreterPath().
   *
   * The default implementation treats some common script extensions ("py", "rb", "sh")
   * and tries to find the interpreter using \ref detectInterpreterPath().
   */
  virtual QString getInterpreterPath(const QString& ext);

public slots:
  /** Starts cmd (which is a list of arguments, the first being the
   * program itself) and blocks until process stopped. The QT event
   * loop is updated regularly so that the GUI doesn't freeze.
   *
   * Read result with QProcess::readStdout() and QProcess::readStderr(),
   * get process exit info with processNormalExit() and processExitStatus().
   *
   * \returns TRUE upon success, FALSE upon failure.
   */
  bool startProcess(QStringList cmd, QByteArray stdindata, QStringList env = QStringList());

  /** Convenient function to be used in the case where program doesn't
   * expect stdin data or if you chose to directly close stdin without writing
   * anything to it. */
  bool startProcess(QStringList cmd, QStringList env = QStringList());

  /** Same as getAllStderr(), except result is returned here as QString. */
  QString readStderrString() {
    return QString::fromLocal8Bit(getAllStderr());
  }
  /** Same as getAllStdout(), except result is returned here as QString. */
  QString readStdoutString() {
    return QString::fromLocal8Bit(getAllStdout());
  }


private slots:
  void ourProcExited();
  void ourProcGotOurStdinData();

private:
  int _runstatus;
  bool mProcessAppEvents;
};



#endif
