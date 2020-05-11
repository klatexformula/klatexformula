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


#include <klfdefs.h>
#include <klfsysinfo.h>
#include <klfutil.h>
#include <klfdatautil.h>

#include "klfbackendinput.h"
#include "klfimplementation.h"

#include <QObject>
#include <QString>
#include <QImage>
#include <QImageReader>
#include <QTemporaryDir>
#include <QVariant>
#include <QVariantMap>




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
const KLFBackendSettings & KLFBackendImplementation::settings() const
{
  // const reference is valid as long as the present object instance exists
  return d->settings;
}


// -----------------------------------------------------------------------------

struct KLFBackendCompilationTaskPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendCompilationTask)
  {
  }

  QList<QTemporaryDir*> temp_dir_objs;

  QMutex cache_mutex;
  QMap<QPair<QString,QByteArray>, QByteArray> cached_output;
  QImage cached_image;
};




KLFBackendCompilationTask::KLFBackendCompilationTask(
    const KLFBackendInput & input, const QVariantMap & parameters,
    const KLFBackendSettings & settings, KLFBackendImplementation * implementation
    )
  : QObject(implementation),
    _input(input), _parameters(parameters), _settings(settings), _implementation(implementation)
{
  KLF_INIT_PRIVATE( KLFBackendCompilationTask ) ;
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
  return _input;
}
const QVariantMap & KLFBackendCompilationTask::parameters() const
{
  // validity of const reference should be good until current instance is deleted
  return _parameters;
}
const KLFBackendSettings & KLFBackendCompilationTask::settings() const
{
  // validity of const reference should be good until current instance is deleted
  return _settings;
}

KLFBackendImplementation * KLFBackendCompilationTask::implementation() const
{
  return _implementation;
}



KLFResultErrorStatus<QString> KLFBackendCompilationTask::createTemporaryDir()
{
  QString ver = KLF_VERSION_STRING;
  ver.replace(".", "x"); // make friendly names with chars in [a-zA-Z0-9]
  // the base name for all our temp files
  QString temptemplate = _settings.temp_dir + "/klftmp"+ver+"-XXXXXX";

  // create the temporary directory
  QTemporaryDir * tdir = new QTemporaryDir(temptemplate);

  if (!tdir->isValid()) {
    // failed to create temporary directory
    delete tdir;
    return klf_result_failure(tr("Failed to create temporary directory in '%1'")
                              .arg(_settings.temp_dir));
  }

  d->temp_dir_objs.append(tdir);

  // TODO: allow to disable this when debugging, e.g. using an environment variable
  tdir->setAutoRemove(false) ; //true);

  QString path = tdir->path();
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
      klfSaveVariantToText(parameters, true)
      );
}


void KLFBackendCompilationTask::storeDataToCache(const QString & format_,
                                                 const QVariantMap & parameters_,
                                                 const QByteArray & data)
{
  QString format = format_.toUpper();
  QVariantMap parameters = canonicalFormatParameters(format, parameters_);
  QPair<QString,QByteArray> cache_key = get_cache_key(format, parameters);
  
  QMutexLocker mutex_locker(& d->cache_mutex);

  if (d->cached_output.contains(cache_key)) {
    klfWarning("Data already exists in cache: for format " << format
               << " with parameters " << parameters) ;
    return;
  }

  d->cached_output[cache_key] = data;
}

QStringList KLFBackendCompilationTask::availableFormats(bool also_formats_via_qimage) const
{
  QStringList fl = availableCompileToFormats();

  if (also_formats_via_qimage) {
    const QList<QByteArray> qimage_fmts = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::const_iterator it = qimage_fmts.constBegin();
         it != qimage_fmts.constEnd(); ++it) {
      fl << QString::fromLatin1(*it);
    }
  }

  return fl;
}

KLFResultErrorStatus<QByteArray>
KLFBackendCompilationTask::getOutput(const QString & format_,
                                     const QVariantMap & parameters_)
{
  QString format = format_.toUpper();
  QVariantMap parameters = canonicalFormatParameters(format, parameters_);
  QPair<QString,QByteArray> cache_key = get_cache_key(format, parameters);

  bool has_in_cache;
  { QMutexLocker mutex_locker(& d->cache_mutex);
    has_in_cache = d->cached_output.contains(cache_key);
  }

  if (!has_in_cache) {
    // we need to create this output format for these parameters
    KLFResultErrorStatus<QByteArray> res = compileToFormat(format, parameters);
    klfDbg("Compiled output to " << format << " with parameters " << parameters
           << ". Result ok ? = " << res.ok ) ;
    if (!res.ok) {
      return res;
    }
    if (!res.result.isEmpty()) {
      QMutexLocker mutex_locker(& d->cache_mutex);
      d->cached_output[cache_key] = res.result;
    }
  }

  QMutexLocker mutex_locker(& d->cache_mutex);

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
  KLFResultErrorStatus<QImage> res = compileToImage();
  if (!res.ok) {
    return res;
  }
  if (!res.result.isNull()) {
    d->cached_image = res.result;
  }
  return d->cached_image;
}

KLFResultErrorStatus<QImage> KLFBackendCompilationTask::compileToImage()
{
  const QSet<QByteArray> ok_formats =
    QSet<QByteArray>::fromList(QImageReader::supportedImageFormats());
  const QStringList avail_fmts = availableFormats();

  QString use_format;

  int j;
  for (j = 0; j < avail_fmts.size(); ++j) {
    if (ok_formats.contains(avail_fmts[j].toUtf8())) {
      use_format = avail_fmts[j];
      break;
    }
  }
  
  if (use_format.isEmpty()) {
    return klf_result_failure("No image format is available for constructing a QImage");
  }

  KLFResultErrorStatus<QByteArray> data_result = getOutput(use_format);

  if (!data_result.ok) {
    return klf_result_failure(data_result.error_msg);
  }

  QImage img;
  img.loadFromData(data_result.result, use_format.toUtf8().constData());

  return img;
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

  KLFResultErrorStatus<QByteArray> result = getOutput(format, parameters);

  if (!result.ok) {
    return klf_result_failure(result.error_msg);
  }

  device->write(result.result);  

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
    fout.setFileName(filename);
    if ( ! fout.open(QIODevice::WriteOnly) ) {
      QString error = tr("Cannot write to file '%1': %2").arg(filename).arg(fout.error());
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
