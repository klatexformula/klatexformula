/***************************************************************************
 *   file klfimplementation.h
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

#ifndef KLFIMPLEMENTATION_H
#define KLFIMPLEMENTATION_H


#include <QObject>
#include <QString>
#include <QColor>
#include <QVariant>
#include <QMap>
#include <QMarginsF>

#include <klfdefs.h>

#include <klfbackendinput.h>




class KLFBackendCompilationTask;
struct KLFBackendImplementationPrivate;

class KLFBackendImplementation : public QObject
{
  Q_OBJECT
public:
  KLFBackendImplementation(QObject * parent);
  virtual ~KLFBackendImplementation();

  void setSettings(const KLFBackendSettings & settings);
  /**
   * \note The returned const reference is valid as long as the present object
   *       instance exists.
   */
  const KLFBackendSettings & settings() const;

  /** \brief Instantiate a compilation task for the given input
   *
   * This method is overridden by subclasses and should return a fresh
   * \ref KLFBackendCompilationTask instance with \a this as its parent.
   *
   * This method may return \a NULL if there was an error while creating this
   * compilation task.
   *
   * Returned instances must be \a new-allocated.  As subclasses of \ref
   * KLFBackendCompilationTask, they are \ref QObject 's; they must be given the
   * \a KLFBackendImplementation given as parent. This way, non-deleted compilation
   * tasks get deleted when the KLFBackendImplementation instance gets destroyed.
   * Furthermore, in this way users have the guarantee that it is safe to \a
   * delete instances returned by this method.
   */
  virtual KLFResultErrorStatus<KLFBackendCompilationTask *>
  createCompilationTask(const KLFBackendInput & input,
                        const QVariantMap & parameters) = 0;

private:
  KLF_DECLARE_PRIVATE(KLFBackendImplementation) ;
};



struct KLFBackendCompilationTaskPrivate;

class KLFBackendCompilationTask : public QObject
{
  Q_OBJECT
public:
  KLFBackendCompilationTask(const KLFBackendInput & input, const QVariantMap & parameters,
                            const KLFBackendSettings & settings,
                            KLFBackendImplementation * implementation);
  virtual ~KLFBackendCompilationTask();

  /** Validity of the const reference is assured within the scope of the current
   * KLFBackendCompilationTask instance.
   */
  const KLFBackendInput & input() const;
  /** Validity of the const reference is assured within the scope of the current
   * KLFBackendCompilationTask instance.
   */
  const QVariantMap & parameters() const;
  /** Validity of the const reference is assured within the scope of the current
   * KLFBackendCompilationTask instance.
   */
  const KLFBackendSettings & settings() const;

  KLFBackendImplementation * implementation() const;

  //! Perform the main compilation step
  /** This function should process the input and run latex to generate some
   * output in one or more suitable format(s).
   *
   * Further formats can also be generated by \ref compileToFormat(), you don't
   * have to generate all available formats here.  The implementation chooses
   * which formats to generate during \ref compile() and which it leaves for
   * future calls to compileToFormat() to produce.  Any format compiled to here
   * is one that does not require extra calls and compilation steps at a later
   * stage when the format is requested; however if too many formats are
   * produced at this step then we might do unnecessary work for formats that
   * won't be needed.  As a guideline, the formats produced during compile()
   * should be the reasonable minimum set of formats required to generate at
   * least a PNG, because we're pretty sure that this format will be requested
   * later anyway.
   *
   * The compiled data in whatever formats generated during compile() should be
   * stored internally by the subclass (e.g. in a private data member) so that
   * they can be returned upon a later call to compileToFormat() if any of these
   * formats are specified.
   */
  virtual KLFErrorStatus compile() = 0;

  /** \brief Obtain output in a given format (with specified parameters)
   *
   * Caches the result for each format and for each set of parameters.
   *
   * Usually it is not necessary to subclass this method; simply override
   * \ref compileToFormat() to provide the output in the required formats.
   *
   * Format names are case-insensitive.  By convention, they should be
   * normalized to all-uppercase.
   */
  virtual KLFResultErrorStatus<QByteArray>
  getOutput(const QString & format, const QVariantMap & parameters = QVariantMap());

  /** \brief Obtain output as a QImage
   *
   * This function calls \ref compileToImage().  The result is cached making
   * future calls to \ref getImage() efficient.  The subclass can also directly
   * specify the image to return here with \ref setQImage().
   */
  virtual KLFResultErrorStatus<QImage> getImage();

  /** \brief List of available formats
   *
   * Return a list of format strings that are valid arguments to \ref
   * getOutput().
   *
   * This is basically a list of the formats we can compile to, plus the save
   * formats provided by \ref QImage which are reported at the end of the list.
   *
   * Subclasses should normally not need to reimplement this function. Instead,
   * they should override \ref availableCompileToFormats() to report formats
   * they can compile to.
   */
  virtual QStringList availableFormats(bool also_formats_via_qimage = true) const;

  /** \brief Save output to the given file in the given format
   *
   * This convenience function calls \ref getOutput() and writes the result to
   * the given \a filename.
   *
   * If \a format is empty, it is guessed from the \a filename extension.  If \a
   * filename is empty, the data is written to the standard output.  If both \a
   * format and \a filename are empty, then the format "PNG" is assumed.
   *
   * If the file exists, it is silently overwritten.
   *
   * Subclasses normally shouldn't have to override this method.
   */
  virtual KLFErrorStatus saveToFile(const QString & filename, const QString & format,
                                    const QVariantMap & parameters = QVariantMap());

  /** \brief Save output to the given file in the given format
   *
   * This convenience function calls \ref getOutput() and writes the result to
   * the given QIODevice.
   *
   * The \a format argument cannot be empty.
   *
   * Subclasses normally shouldn't have to override this method.
   */
  virtual KLFErrorStatus saveToDevice(QIODevice * device, const QString & format,
                                      const QVariantMap & parameters = QVariantMap());

protected:

  const KLFBackendInput _input;
  const QVariantMap _parameters;
  const KLFBackendSettings _settings;
  KLFBackendImplementation * _implementation;

  /** \brief Create a temporary directory to work in
   *
   * Creates a temporary directory using \ref QTemporaryDir inside the temporary
   * directory location set in the settings.  The temporary directory will
   * automatically remove itself when this compilation task object instance is
   * destroyed.
   *
   * Subclasses may in principle request multiple temporary directories, they
   * will all be removed when this compilation task object instance is
   * destroyed.
   *
   * The returned path (upon success) will have a trailing slash (or a trailing
   * backslash on Windows systems).
   */
  virtual KLFResultErrorStatus<QString> createTemporaryDir();


  /** \brief Canonicalize parameters to avoid duplicates in cache
   *
   * This method should return a set of parameters that are functionally
   * equivalent to the given \a parameters for this format.  Here is an
   * opportunity to remove unused parameters and perform other canonical
   * transformations and put them in a canonical form to avoid compiling twice
   * to the same format with equivalent parameters.
   *
   * The default implementation returns the \a parameters unchanged.
   */
  virtual QVariantMap canonicalFormatParameters(const QString & format, const QVariantMap & parameters);

  /** \brief List of formats we can compile to
   *
   * Subclasses should override this function to report all the formats that
   * they can compile to using the function \ref compileToFormat().
   *
   * The order of the returned list should roughly reflect which formats should
   * be preferred for applications and/or how difficult it is to generate these
   * formats.  I.e., for various applications, the first available relevant
   * format will be used.  For instance, when using \c pdflatex, the "PDF"
   * format should probably be specified towards the beginning of the list
   * because it is readily available.
   *
   * Also, the returned order is relevant for \ref getImage().  The first format
   * in the returned list that is a recognized image format (see \ref
   * QImageReader::supportedImageFormats()) will be used when \ref getImage() is
   * called.  The rationale is the following: If \ref availableFormats() returns
   * "JPEG" before "PNG", it is assumed that "JPEG" is cheaper to produce than
   * "PNG", and the QImage's returned by getImage() should use the "JPEG" format
   * instead of "PNG" even if this means giving up on transparency.
   *
   * A list such as ["PDF", "PNG", "SVG", "JPEG", "BMP"] will for instance
   * ensure that getImage() uses "PNG" (with transparency) and that applications
   * that need a vector format will probably go for "PDF" and only query "SVG"
   * if they don't want "PDF".
   */
  virtual QStringList availableCompileToFormats() const = 0;

  /** \brief Create output in the specified format with the given parameters.
   *
   * This method is called by \ref getOutput() to compile the given equation
   * into the specified \a format with the specified \a parameters.
   *
   * The \a format argument has already been forced to upper-case.
   *
   * Subclasses have the option to store the resulting data in the cache
   * directly with \ref registerDataToCache() and return an empty QByteArray.
   */
  virtual KLFResultErrorStatus<QByteArray>
  compileToFormat(const QString & format, const QVariantMap & parameters) = 0;

  /** \brief Generate a QImage and store it in cache.
   *
   * The default implementation calls \ref getOutput() with a suitable format
   * like "PNG" and with an empty \a parameters QVariantMap, and uses the
   * resulting data to construct the QImage.
   *
   * The exact choice of format that we query \ref getOutput() for depends on
   * \ref availableFormats() in the following way.  The list of formats returned
   * by \ref avilableFormats() is examined in sequence until a format is found
   * that can be used to construct a QImage (see
   * QImageReader::supportedImageFormats()).  If there are features (such as
   * transparency) that the QImage should have, make sure that an image format
   * that supports transparency (e.g. PNG) is provided \i before other supported
   * image formats in the list returned by \ref availableFormats().
   *
   * Subclasses have the option to store the resulting data in the cache
   * directly with \ref setImage() and return an empty QImage().
   */
  virtual KLFResultErrorStatus<QImage> compileToImage();

  /** \brief Store the image to be returned by \ref getImage()
   *
   * Stores the image in the cache.
   */
  virtual void setImage(const QImage & image);

  /** \brief Register data format into cache.
   *
   * Any data produced during the compilation process can be stored into the
   * compiled data formats cache using this function.
   *
   * You don't need to call this function for any data returned by \ref
   * compileToFormat() (via a call to \ref getOutput()), it is done
   * automatically.
   */
  virtual void storeDataToCache(const QString & format, const QVariantMap& parameters,
                                const QByteArray & data);

private:
  KLF_DECLARE_PRIVATE(KLFBackendCompilationTask) ;
};








#endif
