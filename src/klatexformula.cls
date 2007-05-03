%% klatexformula.cls

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{klatexformula}
              [2007/05/04 v1.1
 Philippe's special Latex document class for KLatexFormula]

\setlength\paperheight{200mm}
\setlength\paperwidth{270mm}

%%%% PAGE MARGINS
% This is unnecessary as of version KLFV2.0 / klatexformula.clsV1.1
%\usepackage[left=0cm,top=0cm,right=0cm,bottom=0cm,nohead,nofoot]{geometry}

\input{size12.clo}

\setlength\lineskip{1\p@}
\setlength\normallineskip{1\p@}
\renewcommand\baselinestretch{}
\setlength\parskip{0\p@ \@plus \p@}
\@lowpenalty   51
\@medpenalty  151
\@highpenalty 301

\setlength\leftmargini  {0em}
\leftmargin  \leftmargini
\setlength\leftmarginii  {0em}
\setlength\leftmarginiii {0em}
\setlength\leftmarginiv  {0em}
\setlength\leftmarginv  {0em}
\setlength\leftmarginvi {0em}
\setlength  \labelsep  {.5em}
\setlength  \labelwidth{\leftmargini}
\addtolength\labelwidth{-\labelsep}
\@beginparpenalty -\@lowpenalty
\@endparpenalty   -\@lowpenalty
\@itempenalty     -\@lowpenalty
\setlength\arraycolsep{5\p@}
\setlength\tabcolsep{6\p@}
\setlength\arrayrulewidth{.4\p@}
\setlength\doublerulesep{2\p@}
\setlength\tabbingsep{\labelsep}
\skip\@mpfootins = \skip\footins
\setlength\fboxsep{3\p@}
\setlength\fboxrule{.4\p@}
\renewcommand \theequation {}%{\@arabic\c@equation}

\DeclareOldFontCommand{\rm}{\normalfont\rmfamily}{\mathrm}
\DeclareOldFontCommand{\sf}{\normalfont\sffamily}{\mathsf}
\DeclareOldFontCommand{\tt}{\normalfont\ttfamily}{\mathtt}
\DeclareOldFontCommand{\bf}{\normalfont\bfseries}{\mathbf}
\DeclareOldFontCommand{\it}{\normalfont\itshape}{\mathit}
\DeclareOldFontCommand{\sl}{\normalfont\slshape}{\@nomath\sl}
\DeclareOldFontCommand{\sc}{\normalfont\scshape}{\@nomath\sc}
\DeclareRobustCommand*\cal{\@fontswitch\relax\mathcal}
\DeclareRobustCommand*\mit{\@fontswitch\relax\mathnormal}
\newcommand\@pnumwidth{1.55em}
\newcommand\@tocrmarg{2.55em}
\newcommand\@dotsep{4.5}

\pagestyle{empty}

\endinput

%% END OF FILE
