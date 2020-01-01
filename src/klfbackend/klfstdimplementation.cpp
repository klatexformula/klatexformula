/***************************************************************************
 *   file klfstdimplementation.cpp
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

#include <klfutil.h>

#include "klfstdimplementation.h"






// ---------------------------------------------------------

struct KLFBackendDefaultImplementationPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendDefaultImplementation)
  {
  }
};


KLFBackendDefaultImplementation::KLFBackendDefaultImplementation(QObject * parent)
  : KLFBackendImplementation(parent)
{
  KLF_INIT_PRIVATE(KLFBackendDefaultImplementation) ;
}

KLFBackendDefaultImplementation::~KLFBackendDefaultImplementation()
{
  KLF_DELETE_PRIVATE ;
}





KLFResultErrorStatus<KLFBackendCompilationTask *>
KLFBackendDefaultImplementation::createCompilationTask(const KLFBackend::klfInput & input,
                                                 const QVariantMap & parameters)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  ct = new KLFBackendDefaultCompilationTask(input, parameters, d->settings, this);
  return ct;
}



static const QStringList klf_known_latex_engines =
  QStringList() << "latex" << "pdflatex" << "xelatex" << "lualatex";

// static
QStringList KLFBackendDefaultImplementation::knownLatexEngines()
{
  return klf_known_latex_engines;
}




// -----------------------------------------------------------------------------


KLFBackendDefaultCompilationTaskBase::KLFBackendDefaultCompilationTaskBase(
    KLFBackendDefaultCompilationTask *task_,
    const KLFBackendInput & input_,
    const QVariantMap & parameters_,
    const KLFBackendSettings & settings_
    )
  : task(task_), input(input_), parameters(parameters_), settings(settings_),
    compilation_initialized(false)
{
  // by default, executable path will be detected automatically later
  latex_engine_exec = QString();

  // main document class & options. Might break things badly if you go for a
  // weird doc class. You shouldn't need to change this, really.
  document_class = "article";
  document_class_options = "";

  // don't automatically size the page to the equation contents, rather, use
  // this fixed size (in latex points "pt")
  fixed_page_size = QSizeF(-1,-1);

  // which color package to use (\usepackage{xcolor} or
  // \usepackage{color}). Only loaded if nontrivial colors are specified.
  usepackage_color = "\\usepackage{xcolor}";

  // inputenc package to load for encoding support (only for latex & pdflatex
  // engines)
  usepackage_utf8_inputenc = "\\usepackage[utf8]{inputenc}";

  // number of times to re-run latex if we see something like "re-run latex"
  // [to get references right] or something like that
  max_latex_runs = 5;
}

QByteArray KLFBackendDefaultCompilationTaskBase::set_pagesize_latex(
    const QByteArray & ltxw,
    const QByteArray & ltxh
    ) const
{
  const QString & engine = input.engine;

  QByteArray s =
    "\\textwidth=" + ltxw + "\\relax"
    "\\textheight=" + ltxh + "\\relax"
    "\\hsize=" + ltxw + "\\relax"
    "\\vsize=" + ltxh + "\\relax"
    "\\paperwidth=" + ltxw + "\\relax"
    "\\paperheight=" + ltxh + "\\relax"
    "\n";

  if (engine == "latex") {
    return s;
  }
  if (engine == "pdflatex") {
    return s +
      "\\pdfpagewidth=" + ltxw + "\\relax"
      "\\pdfpageheight=" + ltxh + "\\relax"
      "\n";
  }
  if (engine == "xelatex") {
    return s +
      "\\pdfpagewidth=" + ltxw + "\\relax"
      "\\pdfpageheight=" + ltxh + "\\relax"
      "\n";
  }
  if (engine == "lualatex") {
    return s +
      "\\pagewidth=" + ltxw + "\\relax"
      "\\pageheight=" + ltxh + "\\relax"
      "\n";
  }

  klfDbg("Unknown page size commands for engine " << engine) ;
  return s;
}

QByteArray KLFBackendDefaultCompilationTaskBase::set_pagesize_latex(const QSizeF & page_size) const
{
  return 
    ( QString::fromLatin1("\\klfXXXppw=%1pt\\relax\\klfXXXpph=%2pt\\relax")
      .arg(page_size.width(), 0, 'f', 5)
      .arg(page_size.height(), 0, 'f', 5) ).toUtf8()
    + set_pagesize_latex("\\klfXXXppw","\\klfXXXpph")
    ;
}



QByteArray KLFBackendDefaultCompilationTaskBase::generate_template() const
{
  QByteArray s;

  s = "\\documentclass";
  if (!document_class_options.isEmpty()) {
    s += "[" + document_class_options.toUtf8() + "]";
  }
  s += "{" + document_class.toUtf8() + "}\n";

  bool has_fixed_page_size = (fixed_page_size.width() > 0 && fixed_page_size.height() > 0);

  bool has_fg_color = (in.fg_color != QColor(0,0,0));
  bool has_fg_transparency = has_fg_color && (qAlpha(in.fg_color) != 255);
  bool has_bg_color = (qAlpha(in.bg_color) > 0);
  bool has_bg_transparency = has_bg_color && (qAlpha(in.fg_color) != 255);
  bool has_transparency = has_fg_transparency || has_bg_transparency;

  if (has_fg_color || has_bg_color) {
    s += usepackage_color.toUtf8() + "\n";
  }
  if (has_transparency) {
    s += "\\usepackage{pgf}\n";
  }

  // definitions related to our latex-box-magic
  s += "\\newbox\\klfXXXeqnbox"
    "%\n"
    "\\newdimen\\klfXXXw" // tex box width
    "\\newdimen\\klfXXXh" // tex box height
    "\\newdimen\\klfXXXd" // tex box depth
    "\\newdimen\\klfXXXppw" // paper width (box width + margins)
    "\\newdimen\\klfXXXpph" // paper height (box height + box depth + margins)
    "%\n";

  // set all non-page-size-related dimensions
  s += "\\makeatletter"
    "\\oddsidemargin=\\z@\\relax"
    "\\evensidemargin=\\z@\\relax"
    "\\topmargin=\\z@\\relax"
    "\\voffset=-1in\\relax"
    "\\hoffset=-1in\\relax"
    "\\headsep=\\z@\\relax"
    "\\headheight=\\z@\\relax"
    "\\marginparsep=\\z@\\relax"
    "\\footskip=\\z@\\relax"
    "\\parindent=\\z@\\relax"
    "\\parskip=\\z@\\relax"
    "\\topskip=\\z@\\relax"
    "%\n"
    "\\def\\klfXXXdispskips{%\n  "
    "\\abovedisplayskip=\\z@\\relax"
    "\\belowdisplayskip=\\z@\\relax"
    "\\abovedisplayshortskip=\\z@\\relax"
    "\\belowdisplayshortskip=\\z@\\relax}"
    "\\makeatother\\klfXXXdispskips"
    "\n";

  // if we have to set a fixed page size we set the page size in the preamble.
  // If the engine is latex (i.e. to dvi), we need to set the page size here;
  // it will be a dummy size, but we add a comment marker so that this line is
  // changed in a second run of latex.
  if (has_fixed_page_size) {
    s += set_pagesize_latex(fixed_page_size);
  } else if (engine == "latex") {
    s += begin_page_size_marker
      + "\\makeatletter\n"
      + set_pagesize_latex("\\p@","\\p@") // dummy size, will be set on second run
      + "\\makeatother\n";
    + end_page_size_marker
      ;
  }
    
  if (has_fg_color) {
    s += ( QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n")
           .arg(qRed(in.fg_color)/255.0, 0, 'f', 3)
           .arg(qGreen(in.fg_color)/255.0, 0, 'f', 3)
           .arg(qBlue(in.fg_color)/255.0, 0, 'f', 3) ).toUtf8();
  }
  if (has_bg_color) {
    s += ( QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n")
           .arg(qRed(in.bg_color)/255.0, 0, 'f', 3)
           .arg(qGreen(in.bg_color)/255.0, 0, 'f', 3)
           .arg(qBlue(in.bg_color)/255.0, 0, 'f', 3) ).toUtf8();
    if (!has_bg_transparency) {
      s += "\\pagecolor{klfbgcolor}\n";
    }
  }

  s += "\n";
  s += in.preamble.toUtf8();
  s += "\n"
    "\\pagestyle{empty}\n"
    "\\begin{document}%\n"
    ;
  s += "\\setbox\\klfXXXeqnbox\\vbox\\bgroup%\n"
    "\\klfXXXdispskips%\n"
    ;
  if (in.fontsize > 0) {
    s += ( QString::fromLatin1("\\fontsize{%1}{%2}\\selectfont%\n")
           .arg(in.fontsize, 0, 'f', 2)
           .arg(in.fontsize*1.2, 0, 'f', 2) ).toUtf8();
  }
  // the contents of the box -- the latex equation to evaluate, properly
  // enclosed in the given math mode delimiters
  s += in.math_delimiters.first.toUtf8() + "%\n"
    + latexin.toUtf8() + "%\n"
    + in.math_delimiters.second.toUtf8() + "%\n"
    ;

  s += "\\egroup"; // close the temporary box

  // do box size calculations, set dimensions
  s +=
    "\\klfXXXw=\\wd\\klfXXXeqnbox\\relax"
    "\\klfXXXh=\\ht\\klfXXXeqnbox\\relax"
    "\\klfXXXd=\\dp\\klfXXXeqnbox\\relax"
    "\\klfXXXppw=\\klfXXXw\\relax"
    "\\klfXXXpph=\\klfXXXh\\relax"
    "\\advance\\klfXXXpph \\klfXXXd\\relax"
    "%\n"
    ;

  if (!in.margins.isNull()) {
    // we have set some margins
    s += ( QString::fromLatin1(
               "\\advance\\klfXXXppw %1pt\\relax"
               "\\advance\\klfXXXpph %2pt\\relax"
               "\\advance\\hoffset %3pt\\relax"
               "\\advance\\voffset %4pt\\relax"
               "%\n")
           .arg(in.margins.left() + in.margins.right(), 0, 'f', 5)
           .arg(in.margins.top() + in.margins.bottom(), 0, 'f', 5)
           .arg(in.margins.left(), 0, 'f', 5)
           .arg(in.margins.top(), 0, 'f', 5) ).toUtf8()
      ;
  }

  // now, set the page size (unless we had a fixed page size or the engine
  // doesn't support setting the page size after \begin{document}
  if (!has_fixed_page_size && engine != "latex") {
    s += set_pagesize_latex("\\klfXXXppw", "\\klfXXXpph");
  }

  //
  // Display the box. Use appropriate colors/transparency.
  //
  if (has_bg_transparency) {
    // we'll do this by drawing the background rectangle explicitly with
    // pgfsetfillopacity and the requested color.  This requires some \rlap{}
    // tricks.
    s += ( QString::fromLatin1(
               "\\makebox[0pt][l]{%\n"
               "\\color{klfbgcolor}\\pgfsetfillopacity{%1}"
               "\\rule{\\klfXXXppw}{\\klfXXXpph}}"
               ).arg(qAlpha(bg_color)/255.0, 0, 'f', 3) ).toUtf8();
  }
  // now draw the foreground
  if (has_transparency) {
    // do this regardless of has_fg_transparency. If background is
    // transparent, we also need to reset the opacity.
    if (!has_fg_transparency) {
      s += "\\pgfsetfillopacity{1}%\n";
    } else {
      s += ( QString::fromLatin1("\\pgfsetfillopacity{%1}%\n")
             .arg(qAlpha(fg_color)/255.0, 0, 'f', 3) ).toUtf8();
    }
  }
  if (has_fg_color) {
    s += "\\color{klffgcolor}%\n";
  }
  s += "\\raisebox{\\klfXXXd}[\\klfXXXpph][0pt]{\\box\\klfXXXeqnbox}%\n";

  // Finally, instruct latex to provide box size information via messages on
  // standard output
  s += "\\message{%\n" +
    begin_output_meta_info_marker.replace("\n", "^^J%\n") + "%\n"
    "PAPER_WIDTH={\\the\\klfXXXppw}^^J%\n"
    "PAPER_HEIGHT={\\the\\klfXXXpph}^^J%\n"
    "HOFFSET={\\the\\hoffset}^^J%\n"
    "VOFFSET={\\the\\voffset}^^J%\n"
    "BOX_WIDTH={\\the\\klfXXXw}^^J%\n"
    "BOX_HEIGHT={\\the\\klfXXXh}^^J%\n"
    "BOX_DEPTH={\\the\\klfXXXd}^^J%\n" +
    end_output_meta_info_marker.replace("\n", "^^J%\n") + "}%\n"
    ;

  // And that's it!
  s += "\\end{document}\n";

  return s;
}

KLFErrorStatus KLFBackendDefaultCompilerBase::init_compilation()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  compil_tmp_basefn = tmp_klfeqn_dir + "klfeqn";
  klfDbg("Temp location file base name is " << compil_tmp_basefn) ;

  compil_fn_tex = compil_tmp_basefn + ".tex";

  compil_ltx_templatedata = generate_template(input.latex, parameters, input.engine);

  KLFErrorStatus r;

  // write template file
  r = write_file_contents(compil_fn_tex, compil_ltx_templatedata);
  if (!r.ok) {
    return r;
  }

  compilation_initialized = true;

  return klf_result_success();
}

KLFErrorStatus KLFBackendDefaultCompilerBase::run_latex(const QString & fn_out)
{
  // run latex a first time.
  klfDbg("preparing to launch latex.") ;

  QString latex_exec;
  if (latex_engine_exec) {
    latex_exec = latex_engine_exec;
  } else {
    latex_exec = findLatexEngineExecutable(input.engine, settings);
  }
  if (latex_exec.isEmpty()) {
    return klf_result_failure(tr("Cannot find executable for latex engine %1").arg(input.engine));
  }

  // emit a warning if it looks like the executable is in fact a different
  // latex engine
  { const QString latex_exec_basename = QFileInfo(latex_exec).baseName().toLower();
    if (klf_known_latex_engines.contains(latex_exec_basename) &&
        latex_exec_basename != input.engine) {
      klfWarning("Requested engine " << input.engine
                 << " does not seem to correspond to specified executable " << latex_exec) ;
    }
  }

  KLFErrorStatus r;

  bool requires_latex_rerun = false;
  int num_latex_runs = 0;

  do {

    KLFFilterProcess p(input.engine, tempdir.path());
    p.setFromBackendSettings(settings);

    p.setArgv(QStringList() << latex_exec << QDir::toNativeSeparators(fn_tex));

    QByteArray userinputforerrors = "h\nr\n";
    QByteArray ltx_stdout;
    p.collectStdoutTo(&ltx_stdout);

    ok = p.run(userinputforerrors, fn_out, &compil_ltx_output_data);
    if (!ok) {
      return klf_result_failure(QString::fromLatin1("LaTeX (%1): %2")
                                .arg(input.engine, p.resultErrorString()));
    }
    ++ num_latex_runs;

    bool check_for_rerun = true;
    if (num_latex_runs >= max_latex_runs) {
      klfWarning("Maximum number of LaTeX re-runs reached (" << max_latex_runs << "), moving on") ;
      check_for_rerun = false;
    }

    requires_latex_rerun = false;

    r = after_latex_run(compil_ltx_output_data, fn_out, num_latex_runs,
                       check_for_rerun, &requires_latex_rerun);
    if (!r.ok) {
      return r;
    }

  } while (check_for_rerun && requires_latex_rerun) ;

  return klf_result_success();
}


KLFErrorStatus KLFBackendDefaultCompilationTaskBase::after_latex_run(
    const QByteArray & latex_stdout,
    const QString & fn_out,
    int num_latex_runs,
    bool check_for_rerun,
    bool * requires_latex_rerun
    )
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  KLFErrorStatus r;

  if (num_latex_runs < 2 && input.engine == "latex") {
    // read reported latex box size & fix template & re-run

    QRegularExpression rx("\\s*([A-Za-z0-9_.-]+)\\=\\{([^\\}]*)\\}");
    QRegularExpression rx_length_pt("^\\s*([0-9.e+-]+)\\s*pt\\s*$",
                                    QRegularExpression::CaseInsensitiveOption);

    qreal paper_width = 0;
    bool paper_width_set = false;
    qreal paper_height = 0;
    bool paper_height_set = false;

    { // parse reported latex box size
      int i = latex_stdout.indexOf(begin_output_meta_info_marker);
      if (i == -1) {
        return klf_result_failure("Cannot find equation size information that should "
                                  "have been reported by latex");
      }
      // start after the marker
      i += begin_output_meta_info_marker.size();
      
      QRegularExpressionMatch m;
      while ( (m = rx.match(latex_stdout, i, QRegularExpression::NormalMatch,
                            QRegularExpression::AnchoredMatchOption)).hasMatch() ) {
        i = m.capturedEnd(0); // go to next match
        const QString key = m.captured(1);
        const QString value = m.captured(2);
        
        qreal * read_length_to = NULL;
        
        if (key == "PAPER_WIDTH") {
          read_length_to = & paper_width;
          paper_width_set = true;
        } else if (key == "PAPER_HEIGHT") {
          read_length_to = & paper_height;
          paper_height_set = true;
        } else {
          // not an interesting thing to read
          continue;
        }
        // read length into variable *read_length_to
        QRegularExpressionMatch mpt = rx_length_pt.match(value);
        if (!mpt.hasMatch()) {
          klfWarning("Invalid length reported by LaTeX (" << input.engine
                     << "): '" << value << "'") ;
        }
        *read_length_to = mpt.captured(1).toFloat();
      }
    }

    // paper_width & paper_height should be set
    if (!paper_width_set || !paper_height_set) {
      klfWarning("Did not find both PAPER_WIDTH and PAPER_HEIGHT in latex output") ;
      klfDbg("Latex standard output (stdout) was:\n" << latex_stdout) ;
      return klf_result_failure("Could not determine equation's typeset size") ;
    }

    // change latex template to hard-code the final paper size in the preamble
    { int blockstart = compil_ltx_templatedata.indexOf(begin_page_size_marker);
      int blockend = compil_ltx_templatedata.indexOf(end_page_size_marker);
      if (blockstart == -1 || blockend == -1) {
        return klf_result_failure("LaTeX code template does not contain markers "
                                  "for setting fixed page size");
      }
      blockend += end_page_size_marker.size();

      // replace s[blockkstart:blockend] by new page size code
      compil_ltx_templatedata.replace(
          blockstart,
          blockend-blockstart,
          (begin_page_size_marker
           + set_pagesize_latex(QSizeF(paper_width, paper_height))
           + end_page_size_marker)
          );

      // over-write template file
      r = write_file_contents(compil_fn_tex, compil_ltx_templatedata);
      if (!r.ok) {
        return r;
      }
    }
    // all set for a latex re-run
    *requires_latex_rerun = true;
    return klf_result_success();
  }

  if (!check_for_rerun) {
    return klf_result_success();
  }

  // inspect latex output to see if there is anything like "re-run ... [to get
  // stuff right...]"
  { QRegularExpression rx_rerun("LaTeX Warning:.*(\\n.*){,3}.*\\bRe-?run\\b.*",
                                QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = rx_rerun.match(latex_output);
    if (m.hasMatch()) {
      // need to re-run latex for some feature to work
      *requires_latex_rerun = true;
      return klf_result_success();
    }
  }

  // all good, we may proceed
  // *requires_latex_rerun = false; // guaranteed false by default
  return klf_result_success();
}


KLFErrorStatus KLFBackendDefaultCompilerBase::process_ghostscript_to_pdf(
    const QString & fn_input,
    const QString & fn_output
    )
{
  // use ghostscript to process the produced PS or PDF to:
  //   - outline fonts
  //   - add pdfmarks meta-information
  // --> generates our final PDF
  
  ................;
}


KLFErrorStatus KLFBackendDefaultCompilerBase::postprocess_latex_output(const QString & fn_out)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // if "latex" engine, run dvips.
  ...........;

  // regardless of engine, run process_ghostscript_to_pdf() to process PS|PDF
  // output appropriately to generate PDF with meta-information
  ...............;
}



KLFErrorStatus KLFBackendDefaultCompilationTaskBase::write_file_contents(
    const QString & fn, const QByteArray & contents
    )
{
  // write template file
  QFile fo(fn);
  bool r = fo.open(QIODevice::WriteOnly);
  if ( ! r ) {
    return klf_result_failure(
        tr("Cannot open file for writing: '%1'").arg(fn)
        );
  }
  qint64 numbytes = fo.write(contents);
  if (numbytes != contents.size()) {
    return klf_result_failure(tr("Failure writing to '%1': %2")
                              .arg(fn, fo.errorString()));
  }
  return klf_result_success();
}



// static
const QByteArray KLFBackendDefaultCompilationTaskPrivate::begin_page_size_marker =
  "%%%-KLF-FIXED-PAGE-SIZE-BEGIN\n";
const QByteArray KLFBackendDefaultCompilationTaskPrivate::end_page_size_marker =
  "%%%-KLF-FIXED-PAGE-SIZE-END\n";
const QByteArray KLFBackendDefaultCompilationTaskPrivate::begin_output_meta_info_marker =
  "\n***-KLF-META-INFO-BEGIN-***\n";
const QByteArray KLFBackendDefaultCompilationTaskPrivate::end_output_meta_info_marker =
  "\n***-KLF-META-INFO-END-***\n";



KLFBackendDefaultCompilationTask::KLFBackendDefaultCompilationTask(
    const KLFBackendInput & input, const KLFBackendSettings & settings,
    KLFBackendDefaultImplementation * implementation
    )
{
  KLF_INIT_PRIVATE(KLFBackendDefaultCompilationTask) ;

  if (parameters.contains("engine")) {
    d->engine = parameters["engine"].toString();
  }
  if (parameters.contains("latex_engine_exec")) {
    d->latex_engine_exec = parameters["latex_engine_exec"].toString();
  }
  if (parameters.contains("document_class")) {
    d->document_class = parameters["document_class"].toString();
  }
  if (parameters.contains("document_class_options")) {
    d->document_class_options = parameters["document_class_options"].toString();
  }
  if (parameters.contains("fixed_page_size")) {
    d->fixed_page_size = parameters["fixed_page_size"].toSizeF();
  }
  if (parameters.contains("usepackage_color")) {
    d->usepackage_color = parameters["usepackage_color"].toSizeF();
  }

}

KLFBackendDefaultCompilationTask::~KLFBackendDefaultCompilationTask()
{
  KLF_DELETE_PRIVATE ;
}

KLFErrorStatus KLFBackendDefaultCompilationTask::compile()
{
  return d->compile();
}

// protected
QByteArray KLFBackendDefaultCompilationTask::compileToFormat(const QString & format,
                                                             const QVariantMap & parameters)
{
}


