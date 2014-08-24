/***************************************************************************
 *   file klfwinsparkleupdater.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2014 by Philippe Faist
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

#include <string> // C++ wide string for win_sparkle_set_app_details

#include <QWidget>

#include "klfwinsparkleupdater.h"

#include <winsparkle.h>



// see http://qt-project.org/wiki/toStdWStringAndBuiltInWchar


/*! Convert a QString to an std::wstring */
inline std::wstring qToStdWString(const QString &str)
{
#ifdef _MSC_VER
    return std::wstring((const wchar_t *)str.utf16());
#else
    return str.toStdWString();
#endif
}


/*! Convert an std::wstring to a QString */
/*
inline QString stdWToQString(const std::wstring &str)
{
#ifdef _MSC_VER
    return QString::fromUtf16((const ushort *)str.c_str());
#else
    return QString::fromStdWString(str);
#endif
}
*/




// see https://github.com/vslavik/winsparkle/wiki/Basic-Setup


class KLFWinSparkleUpdater::Private
{
};


KLFWinSparkleUpdater::KLFWinSparkleUpdater(QObject * parent, const QString& aUrl)
  : KLFAutoUpdater(parent)
{
  win_sparkle_set_app_details(qToStdWString("KLatexFormula Project").c_str(),
                              qToStdWString("KLatexFormula").c_str(),
                              qToStdWString(KLF_VERSION_STRING).c_str());
  win_sparkle_set_appcast_url(aUrl.toLatin1().constData());
  win_sparkle_init();
}


KLFWinSparkleUpdater::~KLFWinSparkleUpdater()
{
  win_sparkle_cleanup();
}

void KLFWinSparkleUpdater::checkForUpdates(bool inBackground)
{
  if (inBackground) {
    // win_sparkle_check_update_without_ui();
  } else {
    win_sparkle_check_update_with_ui();
  }
}




