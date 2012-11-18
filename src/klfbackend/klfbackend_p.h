/***************************************************************************
 *   file klfbackend_p.h
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

#ifndef KLFBACKEND_P_H
#define KLFBACKEND_P_H


#include "klffilterprocess.h"

struct KLFBackendFilterProgram : public KLFFilterProcess
{
  int resErrCodes[KLFFP_PAST_LAST_VALUE];

  KLFBackendFilterProgram(const QString& title, const KLFBackend::klfSettings * settings, bool isMainThread)
    : KLFFilterProcess(title, settings)
  {
    for (int i = 0; i < KLFFP_PAST_LAST_VALUE; ++i) {
      resErrCodes[i] = i;
    }

    setProcessAppEvents(isMainThread);
  }


  void errorToOutput(KLFBackend::klfOutput * resError)
  {
    if (resultStatus() == 0) {
      klfDbg("No error.") ;
    }
    resError->status = resErrCodes[resultStatus()];
    resError->errorstr = resultErrorString();
  }
};



#endif
