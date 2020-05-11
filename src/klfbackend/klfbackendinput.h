/***************************************************************************
 *   file klfbackendinput.h
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

#ifndef KLFBACENDINPUT_H
#define KLFBACENDINPUT_H


#include <QString>
#include <QColor>
#include <QVariant>
#include <QMap>
#include <QMarginsF>

#include <klfdefs.h>



/** \brief A latex equation along with meta-data like font size, preamble, etc.
 * 
 * This struct represents the general input to the whole latex-to-equation
 * procedure, ie. the LaTeX code, the mathmode to use, the dpi for rendering
 * png, colors etc.
 *
 * Here are the main, base settings.  Individual implementations may have additional
 * arguments that are passed via the \a parameters argument to \ref
 * KLFBackendImplementation::createCompilationTask().
 */
struct KLFBackendInput
{
  //! The default constructor assigns reasonable values to all fields
  KLFBackendInput()
    : latex(), math_delimiters("\\[", "\\]"), preamble(), font_size(-1),
      fg_color(qRgb(0,0,0)), bg_color(qRgba(255,255,255,0)),
      margins(0,0,0,0), dpi(600), vector_scale(1.0), engine("pdflatex"),
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
  QRgb fg_color;

  //! The background color to use
  /** Specifies the background color as given by <tt>qRgba(r, g, b, alpha)</tt>.
   *
   * \warning background alpha value can only be 0 or 255, not any arbitrary value. Any non-zero
   *   value will be considered as 255.
   *
   * \warning (E)PS and PDF formats can't handle transparency.
   */
  QRgb bg_color;

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

  //! LaTeX engine to use
  /** This should be one of "latex", "pdflatex", "xelatex", or "lualatex".
   */
  QString engine;

  //! Convert text into vector outlines
  /** Use this option to produce output that does not embed fonts.  This is
   * useful for export to vector drawing programs such as Inkscape or Adobe
   * Illustrator.
   */
  bool outline_fonts;

  //! Return the current object's field values as a QVariantMap.
  /** The keys are the \i CamelCased version of the fields defined in this
   * class.
   */
  QVariantMap asVariantMap() const;

  //! Load this object from the given field values saved as a QVariantMap
  /** The keys may be camel-cased versions of the keys in this class or the
   * legacy keys set in meta-info saved by KLatexFormula versions <= 4.
   */
  bool loadFromVariantMap(const QVariantMap & values);
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
  bool detectSettings(const QStringList& extra_path = QStringList());


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


  //! does its best to return the latex executable for the given \a engine
  /** Searches first in latex_bin_dir. If it isn't in there, uses \ref
   * findLatexEngineExecutable() to try to guess the location with some
   * predefined system-specific search paths.
   */
  QString getLatexEngineExecutable(const QString & engine) const;



  static QString findLatexEngineExecutable(const QString & engine,
                                           const QStringList & extra_path = QStringList());
  static QString findGsExecutable(const QStringList & extra_path = QStringList());
  static QString findExecutable(const QString & exe_name,
                                const QStringList & exe_patterns, // may contain "%1"s in pattern
                                const QStringList & extra_path = QStringList());
};







#endif
