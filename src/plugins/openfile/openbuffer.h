/***************************************************************************
 *   file plugins/openfile/openbuffer.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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

#ifndef PLUGINS_OPENBUFFER_H
#define PLUGINS_OPENBUFFER_H

#include <QtCore>
#include <QtGui>

#include "klfbackend.h"
#include "klfstyle.h"
#include "klfmainwin.h"


class OpenBuffer : public QObject
{
  Q_OBJECT
public:
  OpenBuffer(KLFMainWin * mainWin, const QString& file, const QString& format, KLFAbstractOutputSaver *saver = NULL)
    : pMainWin(mainWin), pFile(file), pFormat(format), pSaver(saver), pModified(false)
  {
    pPixmap = QPixmap(":/plugindata/openfile/pics/nopreview.png");
  }

  QString file() const { return pFile; }
  QString format() const { return pFormat; }
  QPixmap pixmap() const { return pPixmap; }

  bool ismodified() const { return pModified; }

signals:

  bool modified();

public slots:

  void save()
  {
    pOutput.input = pInput;
    // save our buffer
    pMainWin->saveOutputToFile(pOutput, pFile, pFormat, pSaver);
  }

  void updateInputFromMainWin()
  {
    pModified = true;
    pInput = pMainWin->currentInputState();
    // KLFBackend::klfOutput output = currentKLFBackendOutput();
    // ...
  }

  void activate()
  {
    pMainWin->slotSetLatex(pInput.latex);
    pMainWin->slotLoadStyle(KLFStyle(pInput));
  }

  void evaluated(const KLFBackend::klfOutput& output)
  {
    pOutput = output;
    pPixmap = QPixmap::fromImage(output.result);
  }

private:
  KLFMainWin *pMainWin;
  QString pFile;
  QString pFormat;
  KLFAbstractOutputSaver * pSaver;
  bool pModified;

  KLFBackend::klfInput pInput;
  KLFBackend::klfOutput pOutput;
  QPixmap pPixmap;
};

#endif

