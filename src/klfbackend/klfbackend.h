/***************************************************************************
 *   file klfbackend.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#ifndef KLFBACKEND_H
#define KLFBACKEND_H

#include <qstring.h>
#include <qmemarray.h>
#include <qpixmap.h>


//! Definition of class \ref KLFBackend
/** \file
 * This file defines the \ref KLFBackend class, which is the base engine providing
 * our core functionality of transforming LaTeX code into graphics.
 */

//! The main engine for KLatexFormula
/** The main engine for KLatexFormula, providing core functionality
 * of transforming LaTeX code into graphics.
 *
 * Don't instanciate this class, use the static function
 * \ref KLFBackend::getLatexFormula() to do all the processing.
 *
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLFBackend
{
public:

  //! General settings for KLFBackend::getLatexFormula()
  /** Some global settings to pass on when calling getLatexFormula(). In this struct you specify some system
   * settings, like a temp directory and some paths */
  struct klfSettings {
    QString tempdir; /**< A temporary directory in which we have write access, e.g. <tt>/tmp/</tt> */
    QString klfclspath; /**< location of the 'klatexformula.cls' file: just the directory, don't include filename here */
    QString latexexec; /**< the latex executable, path incl. if not in $PATH */
    QString dvipsexec; /**< the dvips executable, path incl. if not in $PATH */
    QString gsexec; /**< the gs executable, path incl. if not in $PATH */
    QString epstopdfexec; /**< the epstopdf executable, path incl. if not in $PATH. This isn't mandatory to get PNG so
			   * you may leave this to Null or Empty string to drop PDF support in KLFBackend. */
  };

  //! Specific input to KLFBackend::getLatexFormula()
  /** This struct descibes the input of getLatexFormula(), ie. the LaTeX code, the mathmode to use, the dpi for rendering png,
   * colors etc. */
  struct klfInput {
    QString latex; /**< The latex code to render */
    QString mathmode; /**< The mathmode to use. You may pass an arbitrary string containing '...' . '...' will be replaced
		       * by the latex code. Examples are:
		       * \li <tt>\\[ ... \\]</tt>
		       * \li <tt>\$ ... \$</tt>
		       */
    QString preamble; /**< The LaTeX preample, ie the code that appears after '\\documentclass{...}' and
		       * before '\\begin{document}' */
    unsigned long fg_color; /**< The foreground color to use, in format given by <tt>qRgb(r, g, b)</tt>.
			     * You may not specify an alpha value here, it will be ignored. */
    unsigned long bg_color; /**< The background color to use, in format given by <tt>qRgba(r, g, b, alpha)</tt>.
			     * \warning background alpha value can only be 0 or 255, not any arbitrary value. Any non-zero
			     *   value will be considered as 255.
			     * \warning (E)PS and PDF formats can't handle transparency.  */
    int dpi; /**< The dots per inch resolution to use to render the image. This is directly passed to the <tt>-r</tt>
	      * option of ghostscript */
  };

  //! KLFBackend::getLatexFormula() result
  /** This struct contains data that is returned from getLatexFormula(). This includes error handling information,
   * the resulting image (as a QPixmap) as well as data for PNG, (E)PS and PDF files */
  struct klfOutput {
    int status; /**< An integer describing the status of the request. A zero value means success for everything.
		 * A positive value (status>0) means that a program (latex, dvips, ...) returned a non-zero code and
		 * the stderr/stdout are transmitted to \ref errorstr. In that case, errorstr is formatted in HTML suitable
		 * to be used in a QTextBrowser */
    QString errorstr; /**< An explicit error string. If status is positive (ie. latex/dvips/... error) then this text
		       * is HTML-formatted suitable for a QTextBrowser. */

    QPixmap result; /**< The actual result image. */
    QByteArray pngdata; /**< the data for a png file (exact content) */
    QByteArray epsdata; /**< data for an (eps-)postscript file */
    QByteArray pdfdata; /**< data for a pdf file, if \ref klfSettings::epstopdfexec is non-empty. */
  };

  /** The call to process everything.
   *
   * Pass on a valid \ref klfInput input object, as well as a \ref klfSettings
   * object filled with your input and settings, and you will get output in \ref klfOutput.
   *
   * If an error occurs, klfOutput::status is non-zero and klfOutput::errorstr contains an explicit
   * (NOT locale translated) error in human-readable form.
   *
   * Usage example:
   * \code
   *   ...
   *   // these may be declared at some more global scope
   *   KLFBackend::klfSettings settings;
   *   settings.tempdir = "/tmp";
   *   settings.klfclspath = "/opt/kde3/share/apps/klatexformula/";
   *   settings.latexexec = "latex";
   *   settings.dvipsexec = "dvips";
   *   settings.gsexec = "gs";
   *   settings.epstopdfexec = "epstopdf";
   *
   *   KLFBackend::klfInput input;
   *   input.latex = "\int_{\Sigma}\!(\vec{\nabla}\times\vec u)\,d\vec S = \oint_C \vec{u}\cdot d\vec r";
   *   input.mathmode = "\\[ ... \\]";
   *   input.preamble = "\\usepackage{somerequiredpackage}\n";
   *   input.fg_color = qRgb(255, 128, 128); // light blue
   *   input.bg_color = qRgba(0, 0, 80, 255); // dark blue, opaque
   *   input.dpi = 300;
   *
   *   KLFBackend::klfOutput out = KLFBackend::getLatexFormula(input, settings);
   *
   *   myLabel->setPixmap(out.result);
   *
   *   // write contents of 'out.pdfdata' to a file to get a PDF file (for example)
   *   ...
   * \endcode
   */
  static klfOutput getLatexFormula(const klfInput& in, const klfSettings& settings);



private:
  KLFBackend();

  static void cleanup(QString tempfname);

};

#endif
