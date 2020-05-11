/***************************************************************************
 *   file klfstdimplementation.h
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

#ifndef KLFSTDIMPLEMENTATION_H
#define KLFSTDIMPLEMENTATION_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QSizeF>

#include <klfdefs.h>
#include <klfimplementation.h>


class KLFBackendDefaultCompilationTask;

struct KLFBackendDefaultImplementationPrivate;

class KLFBackendDefaultImplementation : public KLFBackendImplementation
{
  Q_OBJECT
public:
  KLFBackendDefaultImplementation(QObject * parent = NULL);
  virtual ~KLFBackendDefaultImplementation();

  KLFResultErrorStatus<KLFBackendCompilationTask *>
  createCompilationTask(const KLFBackendInput & input,
                        const QVariantMap & parameters = QVariantMap());

  static QStringList knownLatexEngines();


  // these do not need to be set explicitly except if the programs are weirdly
  // named or in very unconventional locations
  void setLatexEngineExecPath(const QString & engine, const QString & latex_exec);
  void setDvipsPath(const QString & dvips_exec);

private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultImplementation) ;
};











struct KLFBackendDefaultCompilationTaskPrivate;


//
// New, cleaner compilation implementation as of KLF 5 with the following
// additional goals:
//
//  - get the baseline right, e.g. in HTML export formats
//
//  - native support for pdflatex and {xe|lua}latex
//
class KLFBackendDefaultCompilationTask : public KLFBackendCompilationTask
{
  Q_OBJECT
public:
  KLFBackendDefaultCompilationTask(const KLFBackendInput & input, const QVariantMap & parameters,
                                   const KLFBackendSettings & settings,
                                   KLFBackendDefaultImplementation * implementation);
  virtual ~KLFBackendDefaultCompilationTask();


  void setLatexEngineExec(const QString & latex_engine_exec, const QString & dvips_exec = QString());


protected:
  virtual QStringList availableCompileToFormats() const;

  virtual KLFResultErrorStatus<QByteArray>
  compileToFormat(const QString & format, const QVariantMap & parameters);

  virtual QVariantMap canonicalFormatParameters(const QString & format, const QVariantMap & parameters);

  QString tmp_klfeqn_dir; // with trailing slash guaranteed

  QString latex_engine_exec;
  QString dvips_exec; // only needed if engine == "latex"

  QString document_class;
  QString document_class_options;
  QSizeF fixed_page_size;
  QString usepackage_color;
  QString usepackage_utf8_inputenc;
  QString baseline_valign;

  int max_latex_runs;

  // compilation data
  QString compil_tmp_basefn;
  QString compil_fn_tex;
  QString compil_fn_final_pdf;
  
  QByteArray compil_ltx_templatedata;
  QByteArray compil_ltx_output_data;

  QByteArray data_raw_dvi; // only if engine == "latex"
  QByteArray data_raw_ps; // only if engine == "latex"
  QByteArray data_raw_pdf; // only if engine is pdf-based
  QByteArray data_final_pdf;

  // markers
  static const QByteArray begin_page_size_marker;
  static const QByteArray end_page_size_marker;
  static const QByteArray begin_output_meta_info_marker;
  static const QByteArray end_output_meta_info_marker;

  //! Build latex code for setting page size
  /** Return necessary LaTeX code to fix page size, depending on \a input.engine
   *
   * If \a input.engine is "latex", this code must be in the preamble for it to
   * have any effect.  For engines "pdflatex", "xelatex", "lualatex" the page
   * size can be set after <code>\\begin{document}</code>.
   *
   * The width and height arguments are any LaTeX length (e.g. "\\paperwidth",
   * "1in", ...).
   */
  virtual QByteArray set_pagesize_latex(const QByteArray & ltxw, const QByteArray & ltxh) const;

  //! Overloaded function but paper size given in latex points "pt"
  /**
   * Requires the TeX length registers "\\klfXXXppw" and "\\klfXXXppw=" to be
   * defined already (e.g. "\\newdimen\\klfXXXppw").  Our default template
   * already does this.
   */
  virtual QByteArray set_pagesize_latex(const QSizeF & page_size) const;


  //! Generate default LaTeX template using the saved input, parameters and settings
  virtual QByteArray generate_template() const;

  virtual KLFErrorStatus run_latex();

  virtual KLFErrorStatus after_latex_run(
      const QByteArray & latex_stdout,
      const QString & fn_out,
      int num_latex_runs,
      bool check_for_rerun,
      bool * requires_latex_rerun
      );


  virtual KLFErrorStatus postprocess_latex_output();

  KLFErrorStatus ghostscript_process_pdf(const QString & fn_input);

  QVariantMap get_metainfo_variant() const;
  QMap<QString,QString> get_metainfo_string_map() const;
  //! Utility to write the given contents to the given file
  virtual KLFErrorStatus write_file_contents(const QString & fn, const QByteArray & contents);



  virtual KLFErrorStatus compile(); // overridden


private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultCompilationTask) ;
};





#endif
