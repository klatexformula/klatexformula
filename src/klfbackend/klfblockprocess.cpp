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

#include <qprocess.h>
#include <qapplication.h>
#include <qeventloop.h>

#include "klfblockprocess.h"

KLFBlockProcess::KLFBlockProcess(QObject *p)  : QProcess(p)
{
  connect(this, SIGNAL(wroteToStdin()), this, SLOT(ourProcGotOurStdinData()));
  connect(this, SIGNAL(processExited()), this, SLOT(ourProcExited()));
}


KLFBlockProcess::~KLFBlockProcess()
{
}

void KLFBlockProcess::ourProcGotOurStdinData()
{
  closeStdin();
}

void KLFBlockProcess::ourProcExited()
{
  _runstatus = 1; // exited
}

bool KLFBlockProcess::startProcess(QStringList cmd, QCString str, QStringList *env)
{
  return startProcess(cmd, QByteArray().duplicate(str.data(), str.length()), env);
}

bool KLFBlockProcess::startProcess(QStringList cmd, QStringList *env)
{
  return startProcess(cmd, QByteArray(), env);
}

bool KLFBlockProcess::startProcess(QStringList cmd, QByteArray stdindata, QStringList *env)
{
  _runstatus = 0;

  setArguments(cmd);

  if (! start(env) )
    return false;

  writeToStdin(stdindata);
  // slot ourProcGotOutStdinData() should be called, which closes input

  while (_runstatus == 0) {
    qApp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
  }

  if (_runstatus < 0) { // some error occurred somewhere
    return false;
  }

  return true;
}


#include "klfblockprocess.moc"
