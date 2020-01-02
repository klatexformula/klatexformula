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


#include <QtMath> // qFabs()

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


static QByteArray fmt_len(qreal length)
{
  return QString::number(length, 'f', 5).toLatin1();
}


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
  //
  // We can also set a page width (e.g., to wrap text) and leave page height to
  // -1 for it to be determined automatically
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
    "\\klfXXXppw=" + fmt_len(page_size.width()) + "pt\\relax"
    "\\klfXXXpph=" + fmt_len(page_size.height()) + "pt\\relax"
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

  bool has_fixed_page_width = (fixed_page_size.width() > 0);
  bool has_fixed_page_height = (fixed_page_size.height() > 0);

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
  if (has_fixed_page_width && has_fixed_page_height) {
    s += begin_page_size_marker
      + set_pagesize_latex(fixed_page_size)
      + end_page_size_marker
      ;
  } else if (engine == "latex") {
    // dummy size, will be set on second run
    s += begin_page_size_marker
      + "\\makeatletter\n"
      + set_pagesize_latex("\\p@","\\p@")
      + "\\makeatother\n"
      + end_page_size_marker
      ;
  } else if (has_fixed_page_width) {
    // dummy height will be fixed after \begin{document}
    s += begin_page_size_marker
      + "\\makeatletter\n"
      "\\klfXXXppw=" + fmt_len(fixed_page_size.width()) + "pt\\relax"
      + set_pagesize_latex("\\klfXXXppw","\\p@")
      + "\\makeatother\n"
      + end_page_size_marker
      ;
  } else {
    // also for pdflatex, we need a tiny size to remove left/right margins in
    // display equations
    s += begin_page_size_marker
      + "\\makeatletter\n"
      + set_pagesize_latex("\\p@","\\p@")
      + "\\makeatother\n"
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
    ;
  if (!has_fixed_paper_width) {
    s += "\\klfXXXppw=\\klfXXXw\\relax";
  }
  if (!has_fixed_paper_height) {
    s += "\\klfXXXpph=\\klfXXXh\\relax"
      "\\advance\\klfXXXpph \\klfXXXd\\relax"
      ;
  }
  s += "%\n";

  if (has_fixed_paper_width) {
    // if a fixed paper width is given, then the left & right margin values
    // represent the proportion of space to allocate on each side.
    const qreal pleft =
      (qFabs(margins.left() + margins.right()) <= 1e-6)
      ? 0.5
      : margins.left()/(margins.left()+margins.right())
      ;
    const QString ps = QString::number(pleft, 'g', 6);
    s += "\\hoffset=\\dimexpr-1in+"+ps+"\\klfXXXppw-"+ps+"\\klfXXXw\\relax"
  } else if (margins.left() != 0 || margins.right() != 0) {
    s +=
      "\\advance\\klfXXXppw " + fmt_len(in.margins.left() + in.margins.right()) + "pt\\relax"
      "\\advance\\hoffset " + fmt_len(in.margins.left()) + "pt\\relax"
      ;
  }
  if (has_fixed_paper_height) {
    // if a fixed paper height is given, then the left & right margin values
    // represent the proportion of space to allocate on each side.
    const qreal ptop =
      (qFabs(margins.top() + margins.bottom()) <= 1e-6)
      ? 0.5
      : margins.top()/(margins.top()+margins.bottom())
      ;
    const QString ps = QString::number(ptop, 'g', 6);
    s += "\\voffset=\\dimexpr-1in+"+ps+"\\klfXXXpph-"+ps+"\\klfXXXh-"+ps+"\\klfXXXd\\relax"
  } else if (margins.top() != 0 || margins.top() != 0) {
    s +=
      "\\advance\\klfXXXpph " + fmt_len(in.margins.top() + in.margins.bottom()) + "pt\\relax"
      "\\advance\\voffset " + fmt_len(in.margins.top()) + "pt\\relax"
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

  KLFResultErrorStatus<QString> rs;
  rs = K->createTemporaryDir(); // guaranteed trailing slash
  if (!rs.ok) {
    return klf_result_failure(rs.error_msg);
  }
  tmp_klfeqn_dir = rs.result;

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

    KLFFilterProcess p(input.engine, tmp_klfeqn_dir);
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
      klfDbg("latex needs to be rerun (detected from " << input.engine << " stdout).") ;
      *requires_latex_rerun = true;
      return klf_result_success();
    }
  }

  // all good, we may proceed
  // *requires_latex_rerun = false; // guaranteed false by default
  return klf_result_success();
}


QByteArray klf_generate_pdfmarks(const QMap<QString,QString> & map);

KLFErrorStatus KLFBackendDefaultCompilerBase::ghostscript_process_pdf(
    const QString & fn_input
    )
{
  // use ghostscript to process the produced PS or PDF to:
  //   - outline fonts
  //   - add pdfmarks meta-information
  // --> generates our final PDF

  QString fn_final_pdf = compil_tmp_basefn + "_final.pdf";
  QString fn_pdfmarks = compil_tmp_basefn + ".pdfmarks";

  KLFErrorStatus r;

  // prepare PDFMarks
  { QMap<QString,QString> map = get_metainfo_string_map();
    map["Title"] = in.latex;
    map["Keywords"] = "KLatexFormula KLF LaTeX equation formula";
    map["Creator"] = "KLatexFormula " KLF_VERSION_STRING;

    QByteArray pdfmarkstr = klf_generate_pdfmarks(const QMap<QString,QString> & map)

    r = write_file_contents(fn_pdfmarks, pdfmarkstr);
    if (!r.ok) {
      return r;
    }
    // pdfmarks file is ready.
  }

  // run "gs"
  KLFFilterProcess p("gs", tmp_klfeqn_dir);

  QStringList gsoptions;
  if (input.outline_fonts) {
    gsoptions << "-dNoOutputFonts";
  }

  p.addArgv(settings.gs_exec);
  p.addArgv(QStringList()
            << "-dNOPAUSE" << "-dSAFER" << "-sDEVICE=pdfwrite" << gsoptions
            << "-sOutputFile="+QDir::toNativeSeparators(fn_final_pdf)
            << "-q" << "-dBATCH" << fn_input << fn_pdfmarks);

  QByteArray stdout_data;
  bool ok = p.run(QByteArray(), fn_final_pdf, &stdout_data);
  ................;
}

KLFErrorStatus KLFBackendDefaultCompilerBase::postprocess_latex_output(const QString & fn_out)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // if "latex" engine, run dvips.
  ...........;

  // regardless of engine, run ghostscript_process_pdf() to process PS|PDF
  // output appropriately to generate PDF with meta-information
  ...............;
}




QVariantMap KLFBackendDefaultCompilerBase::get_metainfo_variant() const
{
  QVariantMap vmap;
  vmap["AppVersion"] = QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING);
  vmap["Application"] =
    QObject::tr("Created with KLatexFormula version %1", "KLFBackend::saveOutputToFile")
    .arg(KLF_VERSION_STRING);
  vmap["Software"] = QString::fromLatin1("KLatexFormula " KLF_VERSION_STRING);

  vmap["Implementation"] = "klf5-default";

  const QVariantMap vmapinput = input.asVariantMap();
  for (QVariantMap::const_iterator it = vmapinput.begin(); it != vmapinput.end(); ++it) {
    vmap["Input" + it.key()] = it.value();
  }

  // important legacy keys
  //InputLatex
  vmap["InputMathMode"] = input.math_delimiters.first + " ... " + input.math_delimiters.second;
  //InputPreamble
  //InputFontSize
  //InputFgColor
  //InputBgColor
  vmap["InputDPI"] = vmap["InputDpi"];
  //InputVectorScale

  return vmap;
}
QMap<QString,QString> KLFBackendDefaultCompilerBase::get_metainfo_string_map() const
{
  const QVariantMap vmap = get_metainfo_variant();
  QMap<QString,QString> map;

  for (QVariantMap::const_iterator it = vmap.begin(); it != vmap.end(); ++it) {
    const QVariant value = it.value();
    if (value.type() == QMetaType::QColor) {
      // use format rgba(R,G,B,A) instead of (R G B A) -- don't worry this will
      // still be happily parsed by klfLoadVariantFromText() even in earlier
      // versions of KLF
      QColor c = value.value<QColor>();
      if (c.alpha() < 255) {
        map[it.key()] = QString::fromLatin1("rgba(%1, %2, %3, %4)")
          .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
      } else {
        map[it.key()] = QString::fromLatin1("rgb(%1, %2, %3)")
          .arg(c.red()).arg(c.green()).arg(c.blue());
      }
    } else {
      map[it.key()] = QString::fromLocal8Bit(klfSaveVariantToText(value));
    }
  }
  return map;
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







// -----------------------------------------------------------------------------

// saving meta-info


KLF_EXPORT QByteArray klf_escape_ps_string(const QString& v)
{
  // write escape codes
  int i;
  // if v is just ascii, no need to encode it in unicode
  bool isascii = true;
  for (i = 0; i < v.length(); ++i) {
    if (v[i] < 0 || v[i] > 126) { // \r, \n, \t, etc. will be escaped in a minute, don't worry
      isascii = false;
      break;
    }
  }
  QByteArray vdata;
  if (isascii) {
    vdata = v.toLatin1();
    
    QByteArray escaped;
    for (i = 0; i < vdata.size(); ++i) {
      char c = vdata[i];
      klfDbg("Char: "<<c);
      if (QChar(vdata[i]).isLetterOrNumber() || c == ' ' || c == '.' || c == ',' || c == '/') {
	escaped += c;
      } else if (c == '\n') {
	escaped += "\\n";
      } else if (c == '\r') {
	escaped += "\\r";
      } else if (c == '\t') {
	escaped += "\\t";
      } else if (c == '\\') {
	escaped += "\\\\";
      } else if (c == '(') {
	escaped += "\\(";
      } else if (c == ')') {
	escaped += "\\)";
      } else {
	klfDbg("escaping char: (int)c="<<(int)c<<" (uint)c="<<uint(c)<<", octal="
               <<klfFmtCC("%03o", (uint)c));
	escaped += QString("\\%1").arg((unsigned int)(unsigned char)c, 3, 8, QChar('0')).toLatin1();
      }
    }
    
    return "("+escaped+")";
  }

  // otherwise, do unicode encoding
  
  QTextCodec *codec = QTextCodec::codecForName("UTF-16BE");
  vdata = codec->fromUnicode(v);
  klfDbg("vdata is "<<klfDataToEscaped(vdata));
  
  QByteArray hex;
  for (i = 0; i < (vdata.size()-1); i += 2) {
    hex += klfFmt("%02x%02x ",
                  (unsigned int)(unsigned char)vdata[i],
                  (unsigned int)(unsigned char)vdata[i+1]);
  }
  return "<" + hex + ">";
}



KLF_EXPORT QByteArray klf_generate_pdfmarks(const QMap<QString,QString> & map)
{
  // See the following for more info:
  // https://stackoverflow.com/q/3010015/1694896
  // http://www.justskins.com/forums/adding-metadata-to-pdf-68647.html
  
  QByteArray s;
  s.append(
      // ensure pdfmark symbol defined in postscript
      "/pdfmark where { pop } { /globaldict where { pop globaldict } { userdict } ifelse "
      "/pdfmark /cleartomark load put } ifelse\n"
      // now the proper PDFmarks DOCINFO dictionary
      "[ "
      );

  QRegularExpression rx_invalid_name_char("[^A-Za-z0-9_-.:@]");

  for (QVariantMap::const_iterator it = map.begin(); it != map.end(); ++it) {
    QString key = it.key();
    QByteArray datavalue = klf_escape_ps_string(it.value());

    // make sure k is a valid literal name.  We use a strict sense of valid
    // name; the PostScript reference is incredibly vague and open about what
    // characters are valid in a name.  "All characters except delimiters and
    // white space can appear in names, including characters ordinarily
    // considered to be punctuation. The following are examples of valid names:
    // abc Offset $$ 23A 13-456 a.b $MyDict @pattern . Use care when choosing
    // names that begin with digits.  For example, while 23A is a valid name,
    // 23E1 is a real number, and 23#1 is a radix number token that represents
    // an integer. A '/' (slash—not backslash) introduces a literal name. [...]"
    int j = 0;
    QRegularExpressionMatch m;
    while ( (m = rx_invalid_name_char.match(key, j)).hasMatch() ) {
      j = m.capturedStart(0);
      key.replace(j, 1, "?");
      j += 1;
    }
    
    s.append( "  /" + key.toLatin1() +" " + datavalue + "\n");
  }

  s.append("  /DOCINFO pdfmark\n");
}
