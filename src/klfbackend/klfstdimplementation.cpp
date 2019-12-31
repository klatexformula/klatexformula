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



struct KLFBackendDefaultCompilationTaskPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendDefaultCompilationTask)
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
  }

  QString latex_engine_exec;

  QString document_class;
  QString document_class_options;

  // size, in latex points
  QSizeF fixed_page_size;

  QString usepackage_color;

  friend class KLFBackendDefaultImplementation;

  // markers
  static const QByteArray begin_page_size_marker;
  static const QByteArray end_page_size_marker;
  static const QByteArray begin_output_meta_info_marker;
  static const QByteArray end_output_meta_info_marker;

  // resulting compiled data
  struct DviEngineOutput {
    QByteArray dvi;
    QByteArray raw_ps;
    QByteArray processed_ps;
  };
  struct PdfEngineOutput {
    QByteArray pdf;
    QByteArray raw_pdf;
    QByteArray processed_pdf;
  };

  union EngineOutput {
    DviEngineOutput dviengine;
    PdfEngineOutput pdfengine;
  };

  EngineOutput output_data;

  inline QString set_pagesize_latex(const QByteArray & ltxw, const QByteArray & ltxh)
  {
    const QString & engine = K->input().engine;

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

  inline QByteArray set_fixed_pagesize_latex(const QSizeF & page_size)
  {
    return 
      ( QString::fromLatin1("\\klfXXXppw=%1pt\\relax\\klfXXXpph=%2pt\\relax")
        .arg(page_size.width(), 0, 'f', 5)
        .arg(page_size.height(), 0, 'f', 5) ).toUtf8()
      + set_pagesize_latex("\\klfXXXppw","\\klfXXXpph")
      ;
  }

  //
  // New, cleaner compilation implementation as of KLF 5 with the following
  // additional goals:
  //
  //  - get the baseline right, e.g. in HTML export formats
  //
  //  - native support for pdflatex and {xe|lua}latex
  //

  QByteArray generate_template(const KLFBackendInput & in, const QVariantMap & parameters)
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
      s += set_fixed_pagesize_latex(fixed_page_size);
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
      begin_output_meta_info_marker.replace("\n", "^^J") + "%\n"
      "***-KLF-META-INFO-BEGIN-***^^J%\n"
      "PAPER_WIDTH={\\the\\klfXXXppw}^^J%\n"
      "PAPER_HEIGHT={\\the\\klfXXXpph}^^J%\n"
      "BOX_WIDTH={\\the\\klfXXXw}^^J%\n"
      "BOX_HEIGHT={\\the\\klfXXXh}^^J%\n"
      "BOX_DEPTH={\\the\\klfXXXd}^^J%\n" +
      end_output_meta_info_marker.replace("\n", "^^J") + "}%\n"
      ;

    // And that's it!
    s += "\\end{document}\n";

    return s;
  }

  KLFErrorStatus write_file_contents(const QString & fn, const QByteArray & contents)
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

  KLFErrorStatus compile()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    const KLFBackendInput & input = K->input();
    const KLFBackendSettings & parameters = K->parameters();
    const KLFBackendSettings & settings = K->settings();

    const QString tmpdir = K->createTemporaryDir(); // trailing slash guaranteed

    QString tmpbasefn = tmpdir + "klfeqn";
    klfDbg("Temp location file base name is " << tmpbasefn) ;

    QString fn_tex = tmpbasefn + ".tex";
    QString fn_dvi = tmpbasefn + ".dvi"; // only produced if engine == "latex"
    QString fn_ps  = tmpbasefn + ".ps";  // only produced if engine == "latex"
    QString fn_pdf = tmpbasefn + ".pdf";
    QString fn_processed_pdf = tmpbasefn + "_processed.pdf";
    QString fn_png = tmpbasefn + ".png";

    QByteArray ltx = generate_template(input.latex, parameters, input.engine);

    KLFErrorStatus r;

    // write template file
    r = write_file_contents(fn_tex, ltx);
    if (!r.ok) {
      return r;
    }

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

    bool requires_latex_rerun = false;

    QString fn_out;
    if (engine == "latex") {
      fn_out = fn_dvi;
    } else {
      fn_out = fn_pdf;
    }

    do {

      KLFFilterProcess p(QLatin1String("LaTeX"), tempdir.path());
      p.setFromBackendSettings(settings);

      p.setArgv(QStringList() << latex_exec << QDir::toNativeSeparators(fn_tex));

      QByteArray userinputforerrors = "h\nr\n";
    
      ok = p.run(userinputforerrors, fn_out, &res.dvidata);
      if (!ok) {
        return klf_result_failure("LaTeX: " + p.resultErrorString());
      }

      if (input.engine == "latex") {
        // need to go via DVI; cannot change page size after \begin{document}.  So
        // read box sizes reported by LaTeX via our special code and re-run latex
        // with that fixed page size.
        qreal box_w, box_h, box_d, paper_w, paper_h;
      

        int begin_index = ltx.indexOf(begin_page_size_marker);
        int end_index = ltx.indexOf(end_page_size_marker);
        ltx = ltx.left(begin_index) + set_pagesize_latex(........);
      }

      ...........;

    } while (requires_latex_rerun) ;


  }
};

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


