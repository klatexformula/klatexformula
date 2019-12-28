/***************************************************************************
 *   file klfworkflow.h
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

#ifndef KLFWORKFLOW_H
#define KLFWORKFLOW_H


#include <klfdefs.h>



/** \brief A latex equation along with meta-data like font size, preamble, etc.
 * 
 * This struct represents the general input to the whole latex-to-equation
 * procedure, ie. the LaTeX code, the mathmode to use, the dpi for rendering
 * png, colors etc.
 *
 * Here are the main, base settings.  Individual workflows may have additional
 * arguments that are passed via the \a parameters argument to \ref
 * KLFBackendWorkflow::createCompilationTask().
 */
struct KLFBackendInput
{
  //! The default constructor assigns reasonable values to all fields
  KLFBackendInput()
    : latex(), math_delimiters(), preamble(), font_size(-1),
      fg_color(0x00000000), bg_color(0xffffffff),
      margins(0,0,0,0), dpi(600), vector_scale(1.0),
      outline_fonts(true)
  { }

  //! The latex code to render
  QString latex;

  //! The latex math mode delimiters to use
  /** You may pass an arbitrary pair of strings specifying the delimiters
   * immediately before and after the latex equation content. For example,
   * <code>QPair&lt;QString,QString>("\\[", "\\]")</code>.
   */
  QPair<QString,QString> math_delimiters;

  //! The LaTeX preamble
  /** This is the code that appears after '\\documentclass{...}' and
   * before '\\begin{document}' */
  QString preamble;

  //! Requested font size, in (latex) points.
  /** This is used as e.g. an argument to <code>\\fontsize{...}{...}</code>.
   * If this value is negative, then we leave the default font size.
   */
  double font_size;

  //! The foreground color to use
  /** Specifies the foreground color as given by <tt>qRgb(r, g, b)</tt>.
   * You may not specify an alpha value here (it will be discarded). */
  unsigned long fg_color;

  //! The background color to use
  /** Specifies the background color as given by <tt>qRgba(r, g, b, alpha)</tt>.
   *
   * \warning background alpha value can only be 0 or 255, not any arbitrary value. Any non-zero
   *   value will be considered as 255.
   *
   * \warning (E)PS and PDF formats can't handle transparency.
   */
  unsigned long bg_color;

  //! Margins to add on each side of the equation
  /** Specifies whether to include some space in the image around the equation.
   *
   * Margins are specified as postscript points.
   */
  QMarginsF margins;

  //! The dots per inch resolution of the resulting image
  /** This is used for instance in the <tt>-r</tt> option of Ghostscript (\c
   * gs).
   */
  int dpi;

  //! Scale factor for vector formats
  /** The entire equation will be scaled by this size ratio.  A value of \a 1.0
   * represents the original size, \a 2.0 twice the size, \a 0.5 half the size,
   * etc.
   */
  double vector_scale;

  /** Convert text into vector outlines
   *
   * Use this option to produce output that does not embed fonts.  This is
   * useful for export to vector drawing programs such as Inkscape or Adobe
   * Illustrator.
   */
  bool outline_fonts;
};




//! General settings: LaTeX executable paths, temporary directory, etc.
/**
 */
struct KLFBackendSettings
{
  //! A default constructor assigning empty values to all fields
  /**
   * Use \ref detectSettings() to automatically discover program paths.
   */
  KLFBackendSettings()
    : temp_dir(),
      latex_bin_dir(),
      gs_exec(),
      exec_env(),
      user_script_interpreters()
  { }


  //! Detect system paths automatically
  /** This method looks for temporary directory, latex executables, ghostscript,
   * etc. in system-dependent standard locations.
   *
   * The current KLFBackendSettings instance is updated with the detected
   * settings.  The \a extra_path string can be used to specify additional
   * locations to search for executables, it has the same os-dependent format as
   * the system PATH variable.
   *
   * Returns TRUE on success or FALSE to indicate failure.
   */
  bool detectSettings(const QString& extra_path);


  //! A temporary directory in which we have write access, e.g. <tt>/tmp/</tt>
  QString temp_dir;

  //! Directory containing latex-related executables
  /** Specify the directory containing the latex-related executables such as \a
   * latex, \a pdflatex, \a metafont, etc.
   */
  QString latex_bin_dir;

  //! The Ghostscript \a gs executable to use
  /** This is a full path, or a simple executable name which is searched for in $PATH
   */
  QString gs_exec;
  
  //! Extra environment variables to set
  /** This environment will be provided to processes such as pdflatex, gs, etc.
   * Specify environment variables and their values as a list of strings of the
   * form <tt>"NAME=value"</tt>.
   */
  QStringList exec_env;
  
  /** Path to interpreters to use for different script formats. The key is the filename
   *  extension of the script (e.g. "py"), and the value is the path to the
   *  corresponding interpreter (e.g. "/usr/local/bin/python3")
   */
  QMap<QString,QString> user_script_interpreters;
};











class KLFBackendCompilationTask;
struct KLFBackendWorkflowPrivate;

class KLFBackendWorkflow
{
  Q_OBJECT
public:
  KLFBackendWorkflow(QObject * parent);
  virtual ~KLFBackendWorkflow();

  void setSettings(const KLFBackendSettings & settings);
  KLFBackendSettings settings() const;

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
   * \a KLFBackendWorkflow given as parent. This way, non-deleted compilation
   * tasks get deleted when the KLFBackendWorkflow instance gets destroyed.
   * Furthermore, in this way users have the guarantee that it is safe to \a
   * delete instances returned by this method.
   */
  KLFBackendCompilationTask * createCompilationTask(const KLFBackendInput & input,
                                                    const QVariantMap & parameters) = 0;

private:
  KLF_DECLARE_PRIVATE(KLFBackendWorkflow) ;
};



struct KLFBackendCompilationTaskPrivate;

class KLFBackendCompilationTask
{
  Q_OBJECT
public:
  KLFBackendCompilationTask(const KLFBackendInput & input, const KLFBackendSettings & settings,
                            KLFBackendWorkflow * workflow);
  virtual ~KLFBackendCompilationTask();

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

  /** \brief List of available formats
   *
   * Return a list of format strings that are valid arguments to \ref
   * getOutput().
   */
  virtual QStringList availableFormats() const = 0;

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
  virtual QVariantMap canonicalParameters(const QString & format, const QVariantMap & parameters);

  /** \brief Create output in the specified format with the given parameters.
   *
   * This method is called by \ref getOutput() to compile the given equation
   * into the specified \a format with the specified \a parameters.
   */
  virtual KLFResultErrorStatus<QByteArray>
  compileToFormat(const QString & format, const QVariantMap & parameters) = 0;

private:
  KLF_DECLARE_PRIVATE(KLFBackendCompilationTask) ;
};




#endif
