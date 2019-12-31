/***************************************************************************
 *   file klfimplementation.cpp
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


#include "klfimplementation.h"

#include <QTemporaryDir>




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



struct KLFBackendImplementationPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendImplementation)
  {
  }

  KLFBackendSettings settings;
};


KLFBackendImplementation::KLFBackendImplementation(QObject * parent)
  : QObject(parent)
{
  KLF_INIT_PRIVATE(KLFBackendImplementation) ;
};

KLFBackendImplementation::~KLFBackendImplementation()
{
  KLF_DELETE_PRIVATE ;
}

void KLFBackendImplementation::setSettings(const KLFBackendSettings & settings)
{
  d->settings = settings;
}
KLFBackend::klfSettings KLFBackendImplementation::settings() const
{
  return d->settings;
}


// -----------------------------------------------------------------------------

struct KLFBackendCompilationTaskPrivate
{
  KLFBackendCompilationTask * K;

  KLFBackendCompilationTaskPrivate(
      KLFBackendCompilationTask * K_,
      const KLFBackendInput & input_, const QVariantMap & parameters_,
      const KLFBackendSettings & settings_, KLFBackendImplementation * implementation_)
    : K(_K), input(input_), parameters(parameters_),
      settings(settings_), implementation(implementation_),
      temp_dir_objs()
  {
  }

  KLFBackendImplementation * implementation;

  const KLFBackendInput input;
  const KLFBackendSettings settings;
  const QVariantMap parameters;

  QList<QTemporaryDir*> temp_dir_objs;

  QMutex mutex;
  QMap<QPair<QString,QByteArray>, QByteArray> cached_output;
}




KLFBackendCompilationTask::KLFBackendCompilationTask(
    const KLFBackendInput & input, const QVariantMap & parameters,
    const KLFBackendSettings & settings, KLFBackendImplementation * implementation
    )
  : QObject(implementation)
{
  d = new KLFBackendCompilationTaskPrivate(this, input, parameters, settings, implementation) ;
}

KLFBackendCompilationTask::~KLFBackendCompilationTask()
{
  if (d != NULL) {
    Q_FOREACH(QTemporaryDir * td, d->temp_dir_objs) {
      delete td;
    }
    d->temp_dir_objs.clear();
  }

  if (d != NULL) {
    delete d;
    d = NULL;
  }
}


const KLFBackendInput & KLFBackendCompilationTask::input() const
{
  // validity of const reference should be good until current instance is deleted
  return d->input;
}
const QVariantMap & KLFBackendCompilationTask::parameters() const
{
  // validity of const reference should be good until current instance is deleted
  return d->parameters;
}
const KLFBackendSettings & KLFBackendCompilationTask::settings() const
{
  // validity of const reference should be good until current instance is deleted
  return d->settings;
}

KLFBackendImplementation * KLFBackendCompilationTask::implementation() const
{
  return d->implementation;
}



KLFResultErrorStatus<QString> KLFBackendCompilationTask::createTemporaryDir()
{
  QString ver = KLF_VERSION_STRING;
  ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
  // the base name for all our temp files
  QString temptemplate = d->settings.temp_dir + "/klftmp"+ver+"-XXXXXX";

  // create the temporary directory
  QTemporaryDir * tdir = new QTemporaryDir(temptemplate);

  if (!tdir.isValid()) {
    // failed to create temporary directory
    delete tdir;
    return klf_result_failure(tr("Failed to create temporary directory in '%1'")
                              .arg(d->settings.temp_dir));
  }

  d->temp_dir_objs.append(tdir);

  // TODO: allow to disable this when debugging, e.g. using an environment variable
  tdir->setAutoRemove(true);

  QString path = tdir.path();
  if (!path.endsWith(QString(KLF_DIR_SEP))) {
    // guarantee trailing slash in returned path
    path += QString(KLF_DIR_SEP);
  }
  return path;
}


KLFResultErrorStatus<QByteArray>
KLFBackendCompilationTask::getOutput(const QString & format_,
                                     const QVariantMap & parameters_)
{
  QString format = format_.toUpper();

  QVariantMap parameters = canonicalParameters(format, parameters_);

  QPair<QString,QByteArray> cache_key(
      format,
      QString::fromLatin1(klfSaveVariantToText(parameters, true))
      );

  QMutexLocker mutex_locker(& d->mutex);

  if (! d->cached_output.contains(cache_key)) {
    // we need to create this output format for these parameters
    KLFResultErrorStatus<QByteArray> res = compileToFormat(format, parameters);
    klfDbg("Compiled output to " << format << " with parameters " << parameters
           << ". Result ok ? = " << res.ok ) ;
    if (!res.ok) {
      return res;
    }
    d->cached_output[cache_key] = output;
  }

  return d->cached_output[cache_key];
}


KLFErrorStatus KLFBackendCompilationTask::saveToDevice(QIODevice * device,
                                                       const QString & format_,
                                                       const QVariantMap & parameters)
{
  QString format = format_.toUpper();

  KLFResultErrorStatus result = getOutput(format, parameters);

  if (!result.ok) {
    return klf_result_failure(result.error_msg);
  }

  device->write(data);  

  return true;
}

KLFErrorStatus KLFBackendCompilationTask::saveToFile(const QString& filename,
                                                     const QString& format_,
                                                     const QVariantMap & parameters)
{
  QString format = format_.toUpper();

  // determine format first
  if (format.isEmpty() && !filename.isEmpty()) {
    QFileInfo fi(filename);
    if ( ! fi.suffix().isEmpty() ) {
      format = fi.suffix().trimmed().toUpper();
    }
  }
  if (format.isEmpty()) {
    format = QLatin1String("PNG");
  }

  // got format. choose output now and prepare write
  QFile fout;

  if (filename.isEmpty() || filename == "-") {
    if ( ! fout.open(stdout, QIODevice::WriteOnly) ) {
      QString error = tr("Cannot write to standard output: %1").arg(fout.error());
      klfWarning(error);
      return klf_result_failure(error);
    }
  } else {
    fout.setFileName(fileName);
    if ( ! fout.open(QIODevice::WriteOnly) ) {
      QString error = tr("Cannot write to file '%1': %2").arg(fileName).arg(fout.error());
      klfWarning(error);
      return klf_result_failure(error);
    }
  }

  return saveToDevice(&fout, format, parameters);
}


QVariantMap KLFBackendCompilationTask::canonicalParameters(
    const QString & , const QVariantMap & parameters
    )
{
  return parameters;
}
