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
  QImage cached_image;
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


static inline QPair<QString,QByteArray> get_cache_key(const QString & format,
                                                      const QVariantMap & parameters)
{
  return QPair<QString,QByteArray>(
      format,
      QString::fromLatin1(klfSaveVariantToText(parameters, true))
      );
}


void KLFBackendCompilationTask::storeDataToCache(const QString & format_,
                                                 const QVariantMap & parameters_,
                                                 const QByteArray & data)
{
  QString format = format_.toUpper();
  QVariantMap parameters = canonicalFormatParameters(format, parameters_);
  QPair<QString,QByteArray> cache_key = get_cache_key(format, parameters);
  
  QMutexLocker mutex_locker(& d->mutex);

  if (d->cached_output.contains(cache_key)) {
    klfWarning("Data already exists in cache: for format " << format
               << " with parameters " << parameters) ;
    return;
  }

  d->cached_output[cache_key] = data;
}

KLFResultErrorStatus<QByteArray>
KLFBackendCompilationTask::getOutput(const QString & format_,
                                     const QVariantMap & parameters_)
{
  QString format = format_.toUpper();
  QVariantMap parameters = canonicalFormatParameters(format, parameters_);
  QPair<QString,QByteArray> cache_key = get_cache_key(format, parameters);

  QMutexLocker mutex_locker(& d->mutex);

  if (! d->cached_output.contains(cache_key)) {
    // we need to create this output format for these parameters
    KLFResultErrorStatus<QByteArray> res = compileToFormat(format, parameters);
    klfDbg("Compiled output to " << format << " with parameters " << parameters
           << ". Result ok ? = " << res.ok ) ;
    if (!res.ok) {
      return res;
    }
    if (!res.result.isEmpty()) {
      d->cached_output[cache_key] = output;
    }
  }

  if (! d->cached_output.contains(cache_key)) {
    klfWarning("Internal error: compileToFormat(\""<<format<<"\", "<<parameters << ") returned "
               "nothing but did not store the data into the cache.") ;
    return klf_result_failure("Output was not stored correctly by internal routine.");
  }

  return d->cached_output[cache_key];
}

KLFResultErrorStatus<QImage> KLFBackendCompilationTask::getImage()
{
  if (!d->cached_image.isNull()) {
    return d->cached_image;
  }
  QImage img = compileToImage();
  if (!img.isNull()) {
    d->cached_image = img;
  }
  return d->cached_image;
}

KLFResultErrorStatus<QImage> KLFBackendCompilationTask::compileToImage()
{
  ............;
}

void KLFBackendCompilationTask::setImage(const QImage & image)
{
  d->cached_image = image;
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


QVariantMap KLFBackendCompilationTask::canonicalFormatParameters(
    const QString & , const QVariantMap & parameters
    )
{
  return parameters;
}
