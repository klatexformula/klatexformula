/***************************************************************************
 *   file klfbackendinput.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2019 by Philippe Faist
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

#include "klfbackendinput.h"



// some standard guess settings for system configurations

#ifdef KLF_EXTRA_SEARCH_PATHS
#  define EXTRA_PATHS KLF_EXTRA_SEARCH_PATHS
#else
#  define EXTRA_PATHS ""
#endif


struct klfbackend_exe_names {
  QString prog_name;
  QStringList exe_names;
};

#if defined(Q_OS_WIN)
static klfbackend_exe_names search_latex_exe_names = {
  "latex-engines",
  QStringList()
  // << "%1" << "%1.exe" // exe name itself (& corresponding "XXX.exe") is automatically searched for
};
static klfbackend_exe_names search_gs_exe_names = {
  "gs", QStringList() << "gswin32c.exe" << "gswin64c.exe" << "mgs.exe"
};
static QStringList std_extra_paths =
  klfSplitEnvironmentPath(EXTRA_PATHS_PRE)
  << "C:\\Program Files*\\MiKTeX*\\miktex\\bin"
  << "C:\\texlive\\*\\bin\\win*"
  << "C:\\Program Files*\\gs*\\gs*\\bin"
  ;
#elif defined(Q_OS_MACOS)
static klfbackend_exe_names search_latex_exe_names = {
  "latex-engines",
  QStringList() // exe name itself is automatically searched for
};
static klfbackend_exe_names search_gs_exe_names = {
  "gs", QStringList() // exe name itself is automatically searched for
};
static QStringList std_extra_paths =
  klfSplitEnvironmentPath(EXTRA_PATHS_PRE)
  << "/usr/texbin"
  << "/Library/TeX/texbin"
  << "/usr/local/bin"
  << "/opt/local/bin"
  << "/sw/bin"
  << "/sw/usr/bin"
  ;
#else
static klfbackend_exe_names search_latex_exe_names = {
  "latex-engines",
  QStringList() // exe name itself is automatically searched for
};
static klfbackend_exe_names search_gs_exe_names = {
  "gs", QStringList() // exe name itself is automatically searched for
};
static QStringList std_extra_paths =
  klfSplitEnvironmentPath(EXTRA_PATHS_PRE)
  ;
#endif


const QStringList & get_std_extra_paths()
{
  static QStringList res_std_extra_paths;
  static bool initialized = false;

  if (qApp == NULL) {
    klfWarning("get_std_extra_paths called before qApp is initialized (qApp==NULL still)") ;
    return std_extra_paths;
  }

  if (!initialized) {
    res_std_extra_paths = std_extra_paths.replaceInStrings(
        "@executable_path",
        qApp->applicationDirPath()
        );
    initialized = true;
  }
  return res_std_extra_paths;
}


// static
QString KLFBackendSettings::findLatexEngineExecutable(const QString & engine,
                                                      const QStringList & extra_path)
{
  return findExecutable(engine, search_latex_exe_names.exe_names, extra_path);
}

// static
QString KLFBackendSettings::findGsExecutable(const QStringList & extra_path)
{
  return findExecutable("gs", search_gs_exe_names.exe_names, extra_path);
}

// static
QString KLFBackendSettings::findExecutable(const QString & exe_name,
                                           const QStringList & exe_patterns_,
                                           const QStringList & extra_path)
{
  const QStringList exe_patterns =
    exe_patterns_.replaceInStrings("%1", exe_name)
#ifdef Q_OS_WIN
    << exe_name+".exe"
#endif
    << exe_name
    ;
  
  QStringList std_paths;
  if (extra_path) {
    std_paths.append( extra_path );
  }
  std_paths.append( get_std_extra_paths() );
  
  KLFPathSearcher srch(exe_patterns, std_paths, true);

  QString result = srch.findFirstMatch();

  if (result.isEmpty()) {
    klfWarning("Cannot find executable for latex engine: " << engine << "") ;
  }

  return result;
}




QVariantMap KLFBackendInput::asVariantMap() const
{
  QVariantMap m;

  m["Latex"] = latex;
  m["MathDelimiters"] = QStringList() << math_delimiters.first << math_delimiters.second;
  m["Preamble"] = preamble;
  m["FontSize"] = font_size;
  m["FgColor"] = QColor::fromRgba(fg_color);
  m["BgColor"] = QColor::fromRgba(bg_color);
  m["Margins"] =
    QList<qreal>() << margins.top() << margins.right() << margins.bottom() << margins.left();
  m["Dpi"] = dpi;
  m["VectorScale"] = vector_scale;
  m["Engine"] = engine;
  m["OutlineFonts"] = outline_fonts;

  return m;
}


void KLFBackendInput::loadFromVariantMap(const QVariantMap & map)
{
  latex = vmap.value("Latex", QString());
  ..................;
}






// namespace klf_backend_detail {

// struct GsInfo
// {
//   GsInfo() { }

//   QString version;
//   int version_maj;
//   int version_min;
//   QString help;
//   QSet<QString> available_devices;

//   // cache gs version/help/etc. information (for each gs executable, in case there are several)
//   static QMap<QString,GsInfo> cache;
//   static void initGsCacheForSettings(const KLFBackend::klfSettings & settings);
// };

// static QMap<QString,GsInfo> GsInfo::cache = QMap<QString,GsInfo>();

// static void GsInfo::initGsCacheForSettings(const KLFBackend::klfSettings & settings)
// {
//   KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;

//   if (gsInfo.contains(settings.gsexec)) // info already cached
//     return;

//   if (settings.gsexec.isEmpty()) {
//     // no GS executable given
//     return;
//   }

//   <COPY IN CODE FOR klfbackend.cpp:initGsInfo().....>
// }

// };

