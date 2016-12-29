/***************************************************************************
 *   file klfbackend.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
/* $Id$ */

#ifndef KLFBACKEND_H
#define KLFBACKEND_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QMap>
#include <QVariant>

#include <klfdefs.h>
#include <klfpobj.h>
#include <klfutil.h> // KLFRefPtr

//! Definition of class \ref KLFBackend
/** \file
 * This file defines the \ref KLFBackend class, which is the base engine providing
 * our core functionality of transforming LaTeX code into graphics.
 */

//! No Error.
#define KLFERR_NOERROR 0

//! Failed to create the temporary directory
#define KLFERR_TEMPDIR_FAIL -48
//! No LaTeX formula is specified (empty string)
#define KLFERR_MISSINGLATEXFORMULA -1
//! The \c "..." is missing in math mode string
#define KLFERR_MISSINGMATHMODETHREEDOTS -2
//! Error while opening .tex file for writing
#define KLFERR_TEXWRITEFAIL -3
//! Error while launching the given \c latex program
#define KLFERR_LATEX_NORUN -4
//! obsolete, same as KLFERR_LATEX_NORUN
#define KLFERR_NOLATEXPROG KLFERR_LATEX_NORUN
//! \c latex program did not exit properly (program killed) (see also \ref KLFERR_PROGERR_LATEX)
#define KLFERR_LATEX_NONORMALEXIT -5
//! obsolete, same as KLFERR_LATEX_NONORMALEXIT
#define KLFERR_LATEXNONORMALEXIT KLFERR_LATEX_NONORMALEXIT
//! No .dvi file appeared after runnig \c latex program
#define KLFERR_LATEX_NOOUTPUT -6
//! obsolete, same as KLFERR_LATEX_NOOUTPUT
#define KLFERR_NODVIFILE KLFERR_LATEX_NOOUTPUT
//! Error while opening .dvi file for reading
#define KLFERR_LATEX_OUTPUTREADFAIL -24
//! obsolete, same as KLFERR_LATEX_OUTPUTREADFAIL
#define KLFERR_DVIREADFAIL KLFERR_LATEX_OUTPUTREADFAIL
//! Error while launching the given \c dvips program
#define KLFERR_DVIPS_NORUN -7
//! obsolete, same as KLFERR_DVIPS_NORUN
#define KLFERR_NODVIPSPROG KLFERR_DVIPS_NORUN
//! \c dvips program did not exit properly (program killed) (see also \ref KLFERR_PROGERR_DVIPS)
#define KLFERR_DVIPS_NONORMALEXIT -8
//! obsolete, same as KLFERR_DVIPS_NONORMALEXIT
#define KLFERR_DVIPSNONORMALEXIT KLFERR_DVIPS_NONORMALEXIT
//! no .eps file appeared after running \c dvips program
#define KLFERR_DVIPS_NOOUTPUT -9
//! obsolete, same as KLFERR_DVIPS_NOOUTPUT
#define KLFERR_NOEPSFILE KLFERR_DVIPS_NOOUTPUT
//! Error while opening .eps file for reading
#define KLFERR_DVIPS_OUTPUTREADFAIL -10
//! obsolete, same as KLFERR_DVIPS_OUTPUTREADFAIL
#define KLFERR_EPSREADFAIL KLFERR_DVIPS_OUTPUTREADFAIL
//! Error while reading/parsing %%BoundingBox: in dvips output
#define KLFERR_DVIPS_OUTPUTNOBBOX -11
//! obsolete, same as \ref KLFERR_DVIPS_OUTPUTNOBBOX
#define KLFERR_NOEPSBBOX KLFERR_DVIPS_OUTPUTNOBBOX
//! Program 'gs' cannot be executed to calculate bounding box
#define KLFERR_GSBBOX_NORUN -25
//! Program 'gs' crashed while calculating bbox (see also \ref KLFERR_PROGERR_GSBBOX)
#define KLFERR_GSBBOX_NONORMALEXIT -26
//! Program 'gs' didn't provide any output
#define KLFERR_GSBBOX_NOOUTPUT -27
//! Program 'gs' calculating bbox didn't provide parsable output
#define KLFERR_GSBBOX_NOBBOX -28
//! Program 'gs' cannot be executed to post-process EPS file (page size, outline fonts)
#define KLFERR_GSPOSTPROC_NORUN -29
//! Program 'gs' crashed while post-processing EPS file (see also \ref KLFERR_PROGERR_GSPOSTPROC)
#define KLFERR_GSPOSTPROC_NONORMALEXIT -30
//! Program 'gs' didn't provide any data after post-processing EPS file
#define KLFERR_GSPOSTPROC_NOOUTPUT -22
//! 'gs' cannot outline fonts: need version <= 9.07 (pswrite -dNOCACHE) or >= 9.15 (ps2write -dNoOutputFonts)
#define KLFERR_GSPOSTPROC_NOOUTLINEFONTS -49
//! obsolete, same as KLFERR_GSPOSTPROC_NOOUTPUT
#define KLFERR_NOEPSFILE_OF KLFERR_GSPOSTPROC_NOOUTPUT
//! Couldn't read output provided by 'gs' program after post-processing EPS file
#define KLFERR_GSPOSTPROC_OUTPUTREADFAIL -23
//! obsolete, same as KLFERR_GSPOSTPROC_OUTPUTREADFAIL
#define KLFERR_EPSREADFAIL_OF KLFERR_GSPOSTPROC_OUTPUTREADFAIL
//! Program 'gs' couldn't be executed to generate PNG
#define KLFERR_GSPNG_NORUN -14
//! obsolete, same as \ref KLFERR_GSPNG_NORUN
#define KLFERR_NOGSPROG KLFERR_GSPNG_NORUN
//! Program 'gs' didn't exit noramally (crashed) while generating PNG (see also \ref KLFERR_PROGERR_GSPNG)
#define KLFERR_GSPNG_NONORMALEXIT -15
//! obsolete, same as KLFERR_GSPNG_NONORMALEXIT
#define KLFERR_GSNONORMALEXIT KLFERR_GSPNG_NONORMALEXIT
//! No PNG file appeared after running 'gs'
#define KLFERR_GSPNG_NOOUTPUT -16
//! obsolete, same as KLFERR_GSPNG_NOOUTPUT
#define KLFERR_NOPNGFILE KLFERR_GSPNG_NOOUTPUT
//! Failed to read PNG file produced by 'gs'
#define KLFERR_GSPNG_OUTPUTREADFAIL -17
//! obsolete, same as KLFERR_GSPNG_OUTPUTREADFAIL
#define KLFERR_PNGREADFAIL KLFERR_GSPNG_OUTPUTREADFAIL
//! Error while opening pdfmarks file for writing
#define KLFERR_PDFMARKSWRITEFAIL -47
//! Program 'gs' couldn't be executed to generate PDF
#define KLFERR_GSPDF_NORUN -31
//! Program 'gs' didn't exit noramally (crashed) while generating PDF (see also \ref KLFERR_PROGERR_GSPDF)
#define KLFERR_GSPDF_NONORMALEXIT -19
//! obsolete, same as \ref KLFERR_GSPDF_NONORMALEXIT
#define KLFERR_EPSTOPDFNONORMALEXIT KLFERR_GSPDF_NONORMALEXIT
//! No PDF file appeared after running 'gs'
#define KLFERR_GSPDF_NOOUTPUT -20
//! obsolete, same as \ref KLFERR_GSPDF_NOOUTPUT
#define KLFERR_NOPDFFILE KLFERR_GSPDF_NOOUTPUT
//! Failed to read PDF file produced by 'gs'
#define KLFERR_GSPDF_OUTPUTREADFAIL -21
//! obsolete, same as \ref KLFERR_GSPDF_OUTPUTREADFAIL
#define KLFERR_PDFREADFAIL KLFERR_GSPDF_OUTPUTREADFAIL
//! Failed to query \c gs version
#define KLFERR_NOGSVERSION -32
//! This version of \c gs cannot produce SVG
#define KLFERR_GSSVG_NOSVG -33
//! Program 'gs' couldn't be executed to generate SVG
#define KLFERR_GSSVG_NORUN -34
//! Program 'gs' didn't exit noramally (crashed) while generating SVG (see also \ref KLFERR_PROGERR_GSSVG)
#define KLFERR_GSSVG_NONORMALEXIT -35
//! No SVG file appeared after running 'gs'
#define KLFERR_GSSVG_NOOUTPUT -36
//! Failed to read SVG file produced by 'gs'
#define KLFERR_GSSVG_OUTPUTREADFAIL -37
//! Failed to execute user wrapper script
#define KLFERR_USERSCRIPT_NORUN -38
#define KLFERR_USERSCRIPT_NONORMALEXIT -39
#define KLFERR_USERSCRIPT_NOSCRIPTINFO -40
#define KLFERR_USERSCRIPT_INVALIDSCRIPTINFO -41
#define KLFERR_USERSCRIPT_NOOUTPUT -42
#define KLFERR_USERSCRIPT_OUTPUTREADFAIL -43
#define KLFERR_USERSCRIPT_BADKLFVERSION -44
#define KLFERR_USERSCRIPT_BADSKIPFORMATS -45
#define KLFERR_USERSCRIPT_BADCATEGORY -46
// last error defined: -49



//! \c latex exited with a non-zero status
#define KLFERR_PROGERR_LATEX 1
//! \c dvips exited with a non-zero status
#define KLFERR_PROGERR_DVIPS 2
//! \c gs exited with non-zero status while calculating bbox of EPS file generated by dvips
#define KLFERR_PROGERR_GSBBOX 6
//! \c gs exited with non-zero status while post-processing EPS file (page size, font outlines)
#define KLFERR_PROGERR_GSPOSTPROC 5
//! obsolete, same as \ref KLFERR_PROGERR_GSPOSTPROC
#define KLFERR_PROGERR_GS_OF KLFERR_PROGERR_GSPOSTPROC
//! \c gs exited with a non-zero status while producing PNG
#define KLFERR_PROGERR_GSPNG 3
//! obsolete, same as \ref KLFERR_PROGERR_GSPNG
#define KLFERR_PROGERR_GS KLFERR_PROGERR_GSPNG
//! \c gs exited with non-zero status while producing PDF
#define KLFERR_PROGERR_GSPDF 4
//! obsolete, same as \ref KLFERR_PROGERR_GSPDF
#define KLFERR_PROGERR_EPSTOPDF KLFERR_PROGERR_GSPDF
//! \c gs exited with non-zero status while producing SVG
#define KLFERR_PROGERR_GSSVG 7
//! user wrapper script exited with non-zero status
#define KLFERR_PROGERR_USERSCRIPT 8
// last error defined: 8


//! The main engine for KLatexFormula
/** The main engine for KLatexFormula, providing core functionality
 * of transforming LaTeX code into graphics.
 *
 * Don't instanciate this class, use the static function
 * \ref KLFBackend::getLatexFormula() to do all the processing.
 *
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFBackend
{
public:

  class TemplateGenerator;

  //! General settings for KLFBackend::getLatexFormula()
  /** Some global settings to pass on when calling getLatexFormula(). In this struct you specify
   * some system settings, like a temp directory and some paths
   *
   * \note the \c klfclspath field was removed, because we no longer use klatexformula.cls.
   * */
  struct klfSettings
  {
    /** A default constructor assigning default (empty) values to all fields */
    klfSettings() : tborderoffset(0), rborderoffset(0), bborderoffset(0), lborderoffset(0),
		    calcEpsBoundingBox(true), outlineFonts(true),
		    wantRaw(false), wantPDF(true), wantSVG(true), execenv(),
		    templateGenerator(NULL) { }

    /** A temporary directory in which we have write access, e.g. <tt>/tmp/</tt> */
    QString tempdir;
    /** the latex executable, path incl. if not in $PATH */
    QString latexexec;
    /** the dvips executable, path incl. if not in $PATH */
    QString dvipsexec;
    /** the gs executable, path incl. if not in $PATH */
    QString gsexec;
    /** \deprecated
     * <b>This setting is DEPRECATED and no longer used as of version 3.3.</b> PDF is generated
     * by calling ghostscript directly. This value will be ignored!
     *
     * the epstopdf executable, path incl. if not in $PATH. This isn't mandatory to get PNG so
     * you may leave this to Null or Empty string to instruct getLatexFormula() to NOT attempt to
     * generate PDF. If, though, you do specify an epstopdf executable here, epstopdf errors will
     * be reported as real errors.
     */
    QString epstopdfexec;

    /** The number of postscript points to add to top side of the resulting EPS boundingbox.
     * \note Currently this value is rounded off to the nearest integer. The type \c double is
     *   just an anticipation of hi-resolution bounding box adjustment. */
    double tborderoffset;
    /** The number of postscript points to add to right side of the resulting EPS boundingbox
     * \note Currently this value is rounded off to the nearest integer. The type \c double is
     *   just an anticipation of hi-resolution bounding box adjustment. */
    double rborderoffset;
    /** The number of postscript points to add to bottom side of the resulting EPS boundingbox
     * \note Currently this value is rounded off to the nearest integer. The type \c double is
     *   just an anticipation of hi-resolution bounding box adjustment. */
    double bborderoffset;
    /** The number of postscript points to add to left side of the resulting EPS boundingbox
     * \note Currently this value is rounded off to the nearest integer. The type \c double is
     *   just an anticipation of hi-resolution bounding box adjustment. */
    double lborderoffset;

    /** Call GS to recalculate EPS bounding box instead of assuming dvips' one. This setting
     * is ignored with a non-white or non-transparent background color. */
    bool calcEpsBoundingBox;

    /** Strip away fonts in favor of vectorially outlining them with gs.
     *
     * Use this option to produce output that doens't embed fonts, eg. for Adobe Illustrator.
     *
     * \note This option is ignored when generating SVG with ghostscript and fonts are always
     *   outlined. <i>Reason: otherwise, \c gs just rasterizes the fonts very uglyly (!!).</i>
     */
    bool outlineFonts;

    /** If set to false, then klfOutput::pngdata_raw and klfOutput::epsdata_raw will not be
     * initialized. This is the default, to save resources. */
    bool wantRaw;

    /** If set to false, PDF will not be generated. This is useful if you don't want to waste
     * resources generating a PDF file that you will not need. */
    bool wantPDF;

    /** If set to false, SVG will not be generated. This is useful if you don't want to waste
     * resources generating an SVG file that you will not need. */
    bool wantSVG;

    /** Extra environment variables to set (list of <tt>"NAME=value"</tt>) when executing latex,
     * dvips, gs. */
    QStringList execenv;

    /** The TemplateGenerator object that will be used to generate the base document template.
     * Can be \c NULL, in which case the default generator is used, see
     * \ref DefaultTemplateGenerator. */
    TemplateGenerator *templateGenerator;

    /** Path to interpreters to use for different script formats. The key is the filename
     *  extension of the script (e.g. "py"), and the value is the path to the
     *  corresponding interpreter (e.g. "/usr/bin/python")
     */
    QMap<QString,QString> userScriptInterpreters;
  };

  //! Specific input to KLFBackend::getLatexFormula()
  /** This struct descibes the input of getLatexFormula(), ie. the LaTeX code, the mathmode to use,
   * the dpi for rendering png, colors etc. */
  struct klfInput
  {
    /** A default constructor assigning default values to all fields. */
    klfInput() : fontsize(-1), fg_color(0x00), bg_color(0xffffffff), dpi(600), vectorscale(1.0),
		 bypassTemplate(false) { }
    /** The latex code to render */
    QString latex;
    /** The mathmode to use. You may pass an arbitrary string containing '...' . '...' will be replaced
     * by the latex code. Examples are:
     * \li <tt>\\[ ... \\]</tt>
     * \li <tt>\$ ... \$</tt>
     */
    QString mathmode;
    /** The LaTeX preample, ie the code that appears after '\\documentclass{...}' and
     * before '\\begin{document}' */
    QString preamble;
    /** The foreground color to use, in format given by <tt>qRgb(r, g, b)</tt>.
     * You may not specify an alpha value here, it will be ignored. */

    /** The wanted font size in latex points. If negative, leaves the default font size. */
    double fontsize;

    unsigned long fg_color;
    /** The background color to use, in format given by <tt>qRgba(r, g, b, alpha)</tt>.
     * \warning background alpha value can only be 0 or 255, not any arbitrary value. Any non-zero
     *   value will be considered as 255.
     * \warning (E)PS and PDF formats can't handle transparency.
     */
    unsigned long bg_color;
    /** The dots per inch resolution of the resulting image. This is directly passed to the
     * <tt>-r</tt> option of the \c gs program. */
    int dpi;

    /** Scale factor for vector formats. This is size ratio, not in percent (i.e. original size is 1.0). */
    double vectorscale;

    /** If TRUE, indicates that \c latex contains the whole of the latex code, it should not be included into
     * a default document template.
     *
     * In particular, if TRUE, then \c mathmode and \c preamble are have no effect.
     *
     * This property is FALSE by default. */
    bool bypassTemplate;

    /** \brief A Path to a user script that acts as wrapper around LaTeX
     *
     * In short, we will call this script instead of latex. This script should understand
     * some command-line arguments and environment vars, TO BE DOCUMENTED.
     * \bug ......... DOC ........... & IMPLEMENT ................
     */
    QString userScript;

    /** \brief Arbitrary parameters to pass to user script
     *
     * Only relevant if a \c userScript is set.
     *
     * These parameters will be set as environment variables of the form \c "KLF_ARG_<map-key>".
     */
    QMap<QString,QString> userScriptParam;
  };

  //! KLFBackend::getLatexFormula() result
  /** This struct contains data that is returned from getLatexFormula(). This includes error handling
   * information, the resulting image (as a QImage) as well as data for PNG, (E)PS and PDF files */
  struct klfOutput
  {
    /** \brief A code describing the status of the request.
     *
     * A zero value means success for everything. A positive value means that a program (latex, dvips,
     * ...) returned a non-zero exit code. A negative status indicates another error.
     *
     * \c status will be exactly one of the KLFERR_* constants, defined in \ref klfbackend.h .
     *
     * In every case where status is non-zero, a suitable human-readable error string will be provided in
     * the \ref errorstr field. If status is zero, \ref errorstr will be empty.  */
    int status;

    /** \brief An explicit error string.
     *
     * If \ref status is positive (ie. latex/dvips/gs/epstopdf error) then this text is HTML-formatted
     * suitable for a QTextBrowser. Otherwise, the message is a simple plain text sentence. It contains
     * an empty (actually null) string if status is zero.
     *
     * This string is Qt-Translated with QObject::tr() using \c "KLFBackend" as comment. */
    QString errorstr;

    /** \brief The actual resulting image. */
    QImage result;

    /** \brief The input parameters used to generate this output */
    klfInput input;
    /** \brief The settings that this output was generated with */
    klfSettings settings;

    /** \brief The DVI file data outputted by latex executable */
    QByteArray dvidata;
    /** \brief the data for a png file (exact \c gs output content)
     *
     * This image does NOT contain any meta-data. See also \ref pngdata.
     *
     * This field in output object is only initialized if klfSettings::wantRaw is TRUE.
     */
    QByteArray pngdata_raw;
    /** \brief the data for a png file (re-processed with meta information)
     *
     * The following metadata tags are set in the image:
     * - \c "AppVersion" set to <tt>&quot;KLatexFormula <i>&lt;version></i>&quot;</tt>
     * - \c "Application" set to translated string <tt>&quot;Created with KLatexFormula version
     *   <i>&lt;version></i>&quot;</tt>
     * - \c "Software", set to <tt>&quot;KLatexFormula <i>&lt;version></i>&quot;</tt>
     * - \c "InputLatex", \c "InputMathMode", \c "InputPreamble" are set respectively to the latex code
     *   text, the math mode and the preamble as given in the \ref klfInput object.
     * - \c "InputFgColor" set to <tt>&quot;rgb(<i>&lt;0-255></i>, <i>&lt;0-255></i>, <i>&lt;0-255></i>)&quot;</tt>
     * - \c "InputBgColor" set to <tt>&quot;rgba(<i>&lt;0-255></i>, <i>&lt;0-255></i>, <i>&lt;0-255></i>,
     *   <i>&lt;0-255></i>)&quot;</tt>
     * - \c "InputDPI" set to the Dots Per Inch resolution of the image
     * - \c "SettingsTBorderOffset", \c "SettingsRBorderOffset", \c "SettingsBBorderOffset",
     *   \c "SettingsLBorderOffset", are set to the border offsets in postscript points of the image
     *   (respectively top, right, bottom and left)
     * - \c "SettingsOutlineFonts" set to \c "true" or \c "false" as given in \ref klfSettings::outlineFonts.
     * - ... along with more, see klfbackend.cpp.
     */
    QByteArray pngdata;
    /** \brief data for an (eps-)postscript file.
     *
     * Data is exactly as output by <tt>dvips -E</tt>, without any further processing.
     *
     * This field in output object is only initialized if klfSettings::wantRaw is TRUE. */
    QByteArray epsdata_raw;
    /** \brief data for an (eps-)postscript file.
     *
     * Data as output by <tt>dvips</tt>, with the corrected bounding box as specified in the settings
     * object.
     *
     * This field in output object is only initialized if klfSettings::wantRaw is TRUE. */
    QByteArray epsdata_bbox;
    /** \brief data for an (eps-)postscript file.
     *
     * Fonts are outlined with paths if the setting \c klfSettings::outlineFonts is given. */
    QByteArray epsdata;
    /** \brief data for a pdf file */
    QByteArray pdfdata;
    /** \brief data for a SVG file, if ghostscript supports SVG */
    QByteArray svgdata;
    /** \brief Width in points of the resulting equation */
    double width_pt;
    /** \brief Width in points of the resulting equation */
    double height_pt;
  };

  /** \brief The function that processes everything.
   *
   * Pass on a valid \ref klfInput input object, as well as a \ref klfSettings
   * object filled with your input and settings, and you will get output in \ref klfOutput.
   *
   * If \c isMainThread is set to TRUE (the default), then we assume that we're called by the main
   * application thread, and will call QApplication::processEvents() regularly during process
   * executions to prevent the GUI from freezing. Make sure you set FALSE here if you're not in the
   * GUI thread.
   *
   * If an error occurs, klfOutput::status is non-zero and klfOutput::errorstr contains an explicit
   * error in human-readable form. The latter is Qt-Translated with QObject::tr() with "KLFBackend"
   * comment.
   *
   * Usage example:
   * \code
   *   ...
   *   // this could have been declared at some more global scope
   *   KLFBackend::klfSettings settings;
   *   bool ok = KLFBackend::detectSettings(&settings);
   *   if (!ok) {
   *     // vital program not found
   *     raise_error("error in your system: are latex,dvips and gs installed?");
   *     return;
   *   }
   *
   *   KLFBackend::klfInput input;
   *   input.latex = "\\int_{\\Sigma}\\!(\\vec{\\nabla}\\times\\vec u)\\,d\\vec S ="
   *     " \\oint_C \\vec{u}\\cdot d\\vec r";
   *   input.mathmode = "\\[ ... \\]";
   *   input.preamble = "\\usepackage{somerequiredpackage}\n";
   *   input.fg_color = qRgb(255, 168, 88); // beige
   *   input.bg_color = qRgba(0, 64, 64, 255); // dark turquoise
   *   input.dpi = 300;
   *
   *   KLFBackend::klfOutput out = KLFBackend::getLatexFormula(input, settings);
   *
   *   if (out.status != 0) {
   *     // an error occurred. an appropriate error string is in out.errorstr
   *     display_error_to_user(out.errorstr);
   *     return;
   *   }
   *
   *   myLabel->setPixmap(QPixmap(out.result));
   *
   *   // write contents of 'out.pdfdata' to a file to get a PDF file (for example)
   *   {
   *     QFile fpdf(fname);
   *     fpdf.open(IO_WriteOnly | IO_Raw);
   *     fpdf.writeBlock(out.pdfdata);
   *   }
   *   ...
   * \endcode
   *
   * \note This function is safe for threads; it locks a mutex at the beginning and unlocks
   *   it at the end; so if a call to this function is issued while a first call is already
   *   being processed in another thread, the second waits for the first call to finish.
   *   However, if you are not running this from the main thread, you should be sure to pass
   *   FALSE to \c isMainThread, in order to prevent this function from allowing the
   *   application to process events during process executions.
   */
  static klfOutput getLatexFormula(const klfInput& in, const klfSettings& settings,
				   bool isMainThread = true);

  /** \brief Get a list of available output formats
   *
   * If \c output is non-NULL, then this function is an alias for
   * availableSaveFormats(const klfOutput&).
   *
   * If \c output is NULL, then a list of in principle availble output formats is returned,
   * such as
   * \code QStringList() << "PNG" << "PS" << "EPS" << "DVI" << "PDF" << "SVG" << (qt-image-formats); \endcode
   * In this case, not all the given formats are garanteed to be availble; there may be some klfOutput
   * objects where for example a user script has not provided a way to generate some optional formats
   * like PDF or SVG. Provide a valid pointer to \c output to get an exact list of available formats.
   *
   * \note If Jpeg format is available, only \c "JPEG" will be reported and not \c "JPG".
   */
  static QStringList availableSaveFormats(const klfOutput * output = NULL) ;

  /** \brief Get a list of available output formats for saveOutputToDevice()
   *
   * \note If Jpeg format is available, only \c "JPEG" will be reported and not \c "JPG".
   */
  static QStringList availableSaveFormats(const klfOutput& output) ;

  /** \brief Save the output to image file
   *
   * This function can be used to write output obtained with the \ref getLatexFormula() function,
   * to a file named \c fileName with format \c format.
   *
   * \param output the data to save (e.g. as returned by \ref getLatexFormula() )
   * \param fileName the file name to save to. If empty or equal to \c "-" then standard output is used.
   * \param format the format to use to save to fileName
   * \param errorString if a valid pointer, then when an error occurs this string is set to a
   *   text describing the error.
   *
   * If \c format is an empty string, then format is guessed from filename extension; if no extension is
   * found then format defaults to PNG.
   *
   * \c fileName 's extension is NOT adjusted if it does not match an explicitely given format, for
   * example
   * \code saveOutputToFile(output, "myfile.jpg", "PDF"); \endcode
   * will output PDF data to the file \c "myfile.jpg".
   *
   * If \c errorString is non-NULL, then it is set to a human-readable description of the error that
   * occurred if this function returns FALSE. It is left untouched if success.
   *
   * \return TRUE if success or FALSE if failure.
   *
   * qWarning()s are emitted in case of failure.
   */
  static bool saveOutputToFile(const klfOutput& output, const QString& fileName,
			       const QString& format = QString(), QString* errorString = NULL);

  /** \brief Saves the given output into the given device.
   *
   * Overloaded function, provided for convenience. Behaves very much like \ref saveOutputToFile(),
   * except that the format cannot be guessed.
   */
  static bool saveOutputToDevice(const klfOutput& output, QIODevice *device,
				 const QString& format = QString("PNG"), QString* errorString = NULL);

  /** \brief Detects the system settings and stores the guessed values in \c settings.
   *
   * This function tries to find the latex, dvips, gs, and epstopdf in standard locations on the
   * current platform.
   *
   * Detects gs version to see if SVG is supported, saved in \c wantSVG setting.
   *
   * The temporary directory is set to the system temporary directory.
   *
   * If \c isMainThread is set to TRUE (the default), then we assume that we're called by the main
   * application thread, and will call QApplication::processEvents() regularly during process
   * executions to prevent the GUI from freezing. Make sure you set FALSE here if you're not in the
   * GUI thread.
   */
  static bool detectSettings(klfSettings *settings, const QString& extraPath = QString(),
			     bool isMainThread = true);

  /** \brief Detects additional options (e.g. \ref klfSettings::wantSVG) that depend on
   * specific program versions.
   *
   * \c settings is assumed to be a valid settings object with paths set. This function:
   *
   *   - queries \c gs for whether it has an SVG device and sets \ref klfSettings::wantSVG
   *     accordingly;
   *
   *   - sees if additional environment variables are needed to run the given programs (in
   *     particular for \c mgs.exe ghostscript in MikTeX) and stores those into
   *     \ref klfSettings::execenv. Note that the environment settings already existing in
   *     \c settings->execenv are kept; only those variables for which new values are
   *     detected are updated, or if new declarations are needed they are appended.
   *
   * Possibly in the future it might fill in further optional fields in \ref klfSettings
   * that rely on specific features or versions of the installed programs.
   *
   * \note detectSettings() already calls this function. You do not need to call it again
   * if you're using \ref detectSettings().
   *
   * Returns TRUE or FALSE to indicate success or failure.
   */
  static bool detectOptionSettings(klfSettings *settings, bool isMainThread = true);

  /** \bug ........documentation ........ */
  static QStringList userScriptSettingsToEnvironment(const QMap<QString,QString>& userScriptSettings);


  class TemplateGenerator {
  public:
    TemplateGenerator();
    virtual ~TemplateGenerator();

    virtual QString generateTemplate(const klfInput& input, const klfSettings& settings) = 0;
  };

  class DefaultTemplateGenerator : public TemplateGenerator {
  public:
    DefaultTemplateGenerator();
    virtual ~DefaultTemplateGenerator();

    virtual QString generateTemplate(const klfInput& input, const klfSettings& settings);
  };



private:
  KLFBackend();
};



/** Compare two inputs for equality */
KLF_EXPORT bool operator==(const KLFBackend::klfInput& a, const KLFBackend::klfInput& b);
KLF_EXPORT bool operator==(const KLFBackend::klfSettings& a, const KLFBackend::klfSettings& b);

/** \brief detects any additional settings to environment variables
 *
 * \deprecated Please use \ref KLFBackend::detectOptionSettings instead (starting from KLF
 * 3.3).
 *
 * Detects whether the given values of latex, dvips, gs and epstopdf in the
 * given (initialized) settings \c settings need extra environment set,
 * and sets the \c execenv member of \c settings accordingly.
 *
 * Note that the environment settings already existing in \c settings->execenv are
 * kept; only those variables for which new values are detected are updated, or if
 * new declarations are needed they are appended.
 *
 * \note KLFBackend::detectSettings() already calls this function, you don't
 *   have to call this function manually in that case.
 *
 * \return TRUE (success) or FALSE (failure). Currently there is no reason
 * for failure, and returns always TRUE (as of 3.2.1).
 */
KLF_EXPORT bool klf_detect_execenv(KLFBackend::klfSettings *settings);





KLF_EXPORT QStringList klfSettingsToEnvironmentForUserScript(const KLFBackend::klfSettings& settings);
KLF_EXPORT QStringList klfInputToEnvironmentForUserScript(const KLFBackend::klfInput& in);



/** \brief Reading and writing klfbackend meta-info
 *
 * \note Keys should not start with "KLF". The prefix will be added automatically (or inferred automatically
 *   when reading) by the individual subclasses if applicable.
 */
class KLF_EXPORT KLFAbstractLatexMetaInfo
{
public:
  KLFAbstractLatexMetaInfo();
  virtual ~KLFAbstractLatexMetaInfo();

  virtual QString loadField(const QString& key) = 0;
  virtual void saveField(const QString& key, const QString& value) = 0;

  virtual void saveMetaInfo(const KLFBackend::klfInput& in, const KLFBackend::klfSettings& settings) ;
};

class KLF_EXPORT KLFImageLatexMetaInfo : public KLFAbstractLatexMetaInfo
{
public:
  KLFImageLatexMetaInfo(QImage *img);

  QString loadField(const QString &k);
  void saveField(const QString& k, const QString& v);

private:
  QImage *_w;
};


/** \brief Write metainfo to PDF files via pdfmarks for ghostscript.
 *
 * Example code
 * \code
 *   QString pdfmarkstr;
 *   KLFPdfmarksWriteLatexMetaInfo pdfmetainfo(&pdfmarkstr);
 *   pdfmetainfo.savePDFField("Title", in.latex);
 *   pdfmetainfo.savePDFField("Keywords", "KLatexFormula KLF LaTeX equation formula");
 *   pdfmetainfo.savePDFField("Creator", "KLatexFormula " KLF_VERSION_STRING);
 *   pdfmetainfo.saveMetaInfo(in, settings);
 *   pdfmetainfo.finish();
 *   // now pdfmarkstr contains the pdfmarks string to pass to ghostscript.
 * \endcode
 *
 * \bug integrate PDF meta-info reading here from klfmainwin_p.h
 */
class KLFPdfmarksWriteLatexMetaInfo : public KLFAbstractLatexMetaInfo
{
public:
  KLFPdfmarksWriteLatexMetaInfo(QByteArray * string);

  void saveField(const QString& k, const QString& v);
  void finish();

  //! Saves the field without prepending 'KLF' to the key
  void savePDFField(const QString& k, const QString& v);

  /** \warning NOT IMPLEMENTED. */
  QString loadField(const QString& );

private:
  QByteArray * _s;
};





#endif
