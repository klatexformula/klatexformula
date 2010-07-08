/***************************************************************************
 *   file klflatexsyntaxhighlighter.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
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

#ifndef KLFLATEXSYNTAXHIGHLIGHTER_H
#define KLFLATEXSYNTAXHIGHLIGHTER_H

#include <klfdefs.h>

#include <QObject>
#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>



class KLF_EXPORT KLFLatexSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT
public:
  KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent);
  virtual ~KLFLatexSyntaxHighlighter();

  void setCaretPos(int position);

  virtual void highlightBlock(const QString& text);

  enum { Enabled = 0x01,
	 HighlightParensOnly = 0x02,
	 HighlightLonelyParen = 0x04 };

signals:
  void newSymbolTyped(const QString& symbolName);

public slots:
  void refreshAll();

  /** This clears for example the list of already typed symbols. */
  void resetEditing();

private:

  QTextEdit *_textedit;

  int _caretpos;

  enum Format { FNormal = 0, FKeyWord, FComment, FParenMatch, FParenMismatch, FLonelyParen };

  struct FormatRule {
    FormatRule(int ps = -1, int l = 0, Format f = FNormal, bool needsfocus = false)
      : pos(ps), len(l), format(f), onlyIfFocus(needsfocus) { }
    int pos;
    int len;
    int end() const { return pos + len; }
    Format format;
    bool onlyIfFocus;
  };

  struct ParenItem {
    ParenItem(int ps = -1, bool h = false, char c = 0, bool l = false)
      : pos(ps), highlight(h), ch(c), left(l) { }
    int pos;
    bool highlight;
    char ch;
    bool left; //!< if it's \left( instead of (
  };

  QList<FormatRule> _rulestoapply;

  void parseEverything();

  QTextCharFormat charfmtForFormat(Format f);

  /** symbols that have already been typed and that newSymbolTyped() should not
   * be emitted for any more. */
  QStringList pTypedSymbols;
};




#endif
