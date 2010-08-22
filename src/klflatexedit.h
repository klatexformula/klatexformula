/***************************************************************************
 *   file klflatexedit.h
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

#ifndef KLFLATEXEDIT_H
#define KLFLATEXEDIT_H

#include <klfdefs.h>

#include <QObject>
#include <QTextEdit>
#include <QEvent>
#include <QContextMenuEvent>
#include <QMimeData>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>


class KLFLatexSyntaxHighlighter;
class KLFMainWin;


// ------------------------------------------------




/** \brief A text edit field that edits latex code.
 *
 * Implementation of a QTextEdit to type latex code.
 */
class KLF_EXPORT KLFLatexEdit : public QTextEdit
{
  Q_OBJECT
public:
  KLFLatexEdit(QWidget *mainwin);
  virtual ~KLFLatexEdit();

  KLFLatexSyntaxHighlighter *syntaxHighlighter() { return mSyntaxHighlighter; }

  /** This function may be used to give a pointer to a KLFMainWin that we will call
   * to open data when we get a paste/drop. If they can open the data, then we consider
   * the data pasted. Otherwise, rely on the QTextEdit built-in functionality.
   *
   * This pointer may also be NULL, in which case we will only rely on QTextEdit built-in
   * functionality. */
  void setMainWinDataOpener(KLFMainWin *mainwin) { mMainWin = mainwin; }

signals:
  /** This signal is emitted just before the context menu is shown. If someone wants
   * to add entries into the context menu, then connect to this signal, and append
   * new actions to the \c actionList.
   */
  void insertContextMenuActions(const QPoint& pos, QList<QAction*> *actionList);

public slots:
  /** Sets the current latex code to \c latex.
   *
   * \note this function, unlike QTextEdit::setPlainText(), preserves
   *   undo history.
   */
  void setLatex(const QString& latex);
  void clearLatex();

protected:
  virtual void contextMenuEvent(QContextMenuEvent *event);
  virtual bool canInsertFromMimeData(const QMimeData *source) const;
  virtual void insertFromMimeData(const QMimeData *source);

private:
  KLFLatexSyntaxHighlighter *mSyntaxHighlighter;

  /** This is used to open data if needed */
  KLFMainWin *mMainWin;
};





// ----------------------------------------------


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
