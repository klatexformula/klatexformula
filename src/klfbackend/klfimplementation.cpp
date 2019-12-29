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
  KLF_PRIVATE_HEAD(KLFBackendCompilationTask)
  {
    implementation = NULL;
  }

  KLFBackendImplementation * implementation;

  KLFBackendInput input;
  KLFBackendSettings settings;

  QMutex mutex;
  QMap<QPair<QString,QByteArray>, QByteArray> cached_output;
}




KLFBackendCompilationTask::KLFBackendCompilationTask(
    const KLFBackendInput & input, const KLFBackendSettings & settings,
    KLFBackendImplementation * implementation
    )
  : QObject(implementation)
{
  KLF_INIT_PRIVATE(KLFBackendCompilationTask) ;
  d->implementation = implementation;
  d->input = input;
  d->settings = settings;
}

KLFBackendCompilationTask::~KLFBackendCompilationTask()
{
  KLF_DELETE_PRIVATE ;
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


KLFErrorStatus KLFBackendImplementation::saveToDevice(QIODevice * device,
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

KLFErrorStatus KLFBackendImplementation::saveToFile(const QString& filename,
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
