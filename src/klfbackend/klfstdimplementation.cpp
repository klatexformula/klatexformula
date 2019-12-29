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

#include "klfstdimplementation.h"









// ---------------------------------------------------------

struct KLFBackendDefaultImplementationPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendDefaultImplementation)
  {
  }



  //
  // New, cleaner compilation implementation as of KLF 5 with the following additional
  // goals:
  //
  //  - get the baseline right, e.g. in HTML export formats
  //
  //  - native use of pdflatex and {xe|lua}latex
  //

  QString generate_template(const KLFBackend::klfInput & in, const QVariantMap & parameters)
  {
    QString s;
    s += "\\documentclass{article}\n";

    bool has_fg_color = (in.fg_color != QColor(0,0,0));
    bool has_bg_color = (qAlpha(in.bg_color) > 0);

    if (has_fg_color || has_bg_color) {
      s += "\\usepackage{xcolor}\n";
    }

    s += in.preamble;
    s += "\n"
      "\\begin{document}\n"
      "\\thispagestyle{empty}\n";

    if (in.fontsize > 0) {
      s += QString("\\fontsize{%1}{%2}\\selectfont\n")
        .arg(in.fontsize, 0, 'f', 2).arg(in.fontsize*1.2, 0, 'f', 2);
    }
    if (has_fg_color) {
      s += QString("\\definecolor{klffgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.fg_color)/255.0)
        .arg(qGreen(in.fg_color)/255.0).arg(qBlue(in.fg_color)/255.0);
    }
    if (has_bg_color) {
      s += QString("\\definecolor{klfbgcolor}{rgb}{%1,%2,%3}\n").arg(qRed(in.bg_color)/255.0)
        .arg(qGreen(in.bg_color)/255.0).arg(qBlue(in.bg_color)/255.0);
      s += "\\pagecolor{klfbgcolor}\n";
    }
    s += "\\color{klffgcolor} ";
    s += in.math_delimiters.first + "%\n" + latexin + "%\n" + in.math_delimiters.second + "%\n";
    s += "\\end{document}\n";

    return s;
  }

  KLFResultErrorStatus<KLFBackendCompilationTask> *
  create_compilation_task(const KLFBackend::klfInput & input, const QVariantMap & parameters)
  {
    ....
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





KLFBackendCompilationTask *
KLFBackendDefaultImplementation::createCompilationTask(const KLFBackend::klfInput & input,
                                                 const QVariantMap & parameters)
{
  return d->create_compilation_task(input, parameters);
}






// -----------------------------------------------------------------------------



struct KLFBackendDefaultCompilationTaskPrivate
{
  KLF_PRIVATE_HEAD(KLFBackendDefaultCompilationTask)
  {
  }
};


KLFBackendDefaultCompilationTask::KLFBackendDefaultCompilationTask(
    const KLFBackendInput & input, const KLFBackendSettings & settings,
    KLFBackendDefaultImplementation * implementation
    )
{
  KLF_INIT_PRIVATE(KLFBackendDefaultCompilationTask) ;
}

KLFBackendDefaultCompilationTask::~KLFBackendDefaultCompilationTask()
{
  KLF_DELETE_PRIVATE ;
}

// protected
QByteArray KLFBackendDefaultCompilationTask::compileToFormat(const QString & format,
                                                             const QVariantMap & parameters)
{
}


