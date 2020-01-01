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


#include <klfdefs.h>
#include <klfimplementation.h>


class KLFBackendDefaultCompilationTask;

struct KLFBackendDefaultImplementationPrivate;

class KLFBackendDefaultImplementation : public KLFBackendImplementation
{
  Q_OBJECT
public:
  KLFBackendDefaultImplementation(QObject * parent);
  virtual ~KLFBackendDefaultImplementation();

  KLFResultErrorStatus<KLFBackendCompilationTask *>
  createCompilationTask(const KLFBackendInput & input, const QVariantMap & parameters);

  static QStringList knownLatexEngines();

private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultImplementation) ;
};










//
// New, cleaner compilation implementation as of KLF 5 with the following
// additional goals:
//
//  - get the baseline right, e.g. in HTML export formats
//
//  - native support for pdflatex and {xe|lua}latex
//
class KLFBackendDefaultCompilerBase
{
protected:
  const KLFBackendInput & input;
  const QVariantMap & parameters;
  const KLFBackendSettings & settings;

  const QString tmp_klfeqn_dir; // with trailing slash guaranteed

  QString latex_engine_exec;

  QString document_class;
  QString document_class_options;

  // size, in latex points
  QSizeF fixed_page_size;

  QString usepackage_color;

  int max_latex_runs;

  // compilation data
  bool compilation_initialized;
  QString compil_tmp_basefn;
  QString compil_fn_tex;

  QByteArray compil_ltx_templatedata;
  QByteArray compil_ltx_output_data;


  friend class KLFBackendDefaultImplementation;


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

  virtual KLFErrorStatus init_compilation();

  virtual KLFErrorStatus run_latex(const QString & fn_out);

  //! Utility to write the given contents to the given file
  virtual KLFErrorStatus write_file_contents(const QString & fn, const QByteArray & contents);

public:
  KLFBackendDefaultCompilationTaskBase(KLFBackendDefaultCompilationTask *task_,
                                       const KLFBackendInput & input_,
                                       const QVariantMap & parameters_,
                                       const KLFBackendSettings & settings_);


  KLFErrorStatus compile()
  {..............
  }

};





struct KLBackendDefaultCompilationTaskPrivate;

class KLFBackendDefaultCompilationTask : public KLFBackendCompilationTask
{
  Q_OBJECT
public:
  KLFBackendDefaultCompilationTask(const KLFBackendInput & input, const KLFBackendSettings & settings,
                                   KLFBackendDefaultImplementation * implementation);
  virtual ~KLFBackendDefaultCompilationTask();

  virtual QStringList availableFormats() const;

protected:
  virtual KLFResultErrorStatus<QByteArray>
  compileToFormat(const QString & format, const QVariantMap & parameters);

  virtual QVariantMap canonicalParameters(const QString & format, const QVariantMap & parameters);

private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultCompilationTask) ;
}





#endif
