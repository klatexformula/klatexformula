/***************************************************************************
 *   file klflatexpreviewthread.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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

#ifndef klflatexpreviewthread_H__
#define klflatexpreviewthread_H__

#include <QThread>
#include <QMutex>

#include <klfdefs.h>
#include <klfbackend.h>


class KLFLatexPreviewThreadPrivate;

/**
 * A helper that runs in a different thread that generates previews in real-time as user types text,
 * without blocking the GUI.
 *
 * \note returns (max.) three different image sizes set by setPreviewSize(), setLargePreviewSize() and
 *   the original image size. Setting an invalid size disables image generation of that particular preview
 *   size (might save resources by not resizing the image to that size).
 */
class KLF_EXPORT KLFLatexPreviewThread : public QThread
{
  Q_OBJECT

  Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize) ;
  Q_PROPERTY(QSize largePreviewSize READ largePreviewSize WRITE setLargePreviewSize) ;

public:
  KLFLatexPreviewThread(QObject *parent = NULL);
  virtual ~KLFLatexPreviewThread();

  KLFBackend::klfInput intput() const;
  KLFBackend::klfSettings settings() const;

  KLF_PROPERTY_GET(QSize previewSize) ;
  KLF_PROPERTY_GET(QSize largePreviewSize) ;

  void stop();

signals:
  /** Emitted whenever there is no preview to generate (input latex string empty) */
  void previewReset();

  /** Emitted when a preview was successfully generated (i.e., <tt>output.status==0</tt>). The full
   * KLFBackend::klfOutput object is given here. */
  void outputAvailable(const KLFBackend::klfOutput& output) ;
  /** Emitted when a preview was successfully generated. All three images are given here (preview
   * size, large preview size, original image) */
  void previewAvailable(const QImage& preview, const QImage& largePreview, const QImage& fullPreview);
  /** Emitted when a preview was successfully generated. Preview Size image. See also
   * \ref setPreviewSize(). */
  void previewImageAvailable(const QImage& preview);
  /** Emitted when a preview was successfully generated. Large preview size image. See also
   * \ref setLargePreviewSize(). */
  void previewLargeImageAvailable(const QImage& largePreview);
  /** Emitted when a preview was successfully generated. The original image is given. */
  void previewFullImageAvailable(const QImage& fullPreview);

  /** Emitted when generation of the latex preview raised an error. See the error codes
   * defined in klfbackend.h */
  void previewError(const QString& errorString, int errorCode);

public slots:
  /** \returns TRUE if the input was set, FALSE if current input is already equal to \c input */
  bool setInput(const KLFBackend::klfInput& input);
  /** \returns TRUE if the settings were set, FALSE if current settings are already equal to \c settings */
  bool setSettings(const KLFBackend::klfSettings& settings, bool disableExtraFormats = true);
  /** \returns TRUE if the previewSize was set, FALSE if current preview size is already equal to
   * \c previewSize */
  bool setPreviewSize(const QSize& previewSize);
  /** \returns TRUE if the largePreviewSize was set, FALSE if current large preview size is already equal to
   * \c largePreviewSize */
  bool setLargePreviewSize(const QSize& largePreviewSize);

protected:
  virtual void run();

private:
  KLF_DECLARE_PRIVATE(KLFLatexPreviewThread) ;

  QMutex _mutex;
};



#endif
