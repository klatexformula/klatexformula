/***************************************************************************
 *   file klflatexedit.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
class KLFDropDataHandler;


// ------------------------------------------------



/** \brief A text edit field that edits latex code.
 *
 * Implementation of a QTextEdit to type latex code.
 */
class KLF_EXPORT KLFLatexEdit : public QTextEdit
{
  Q_OBJECT

  Q_PROPERTY(int heightHintLines READ heightHintLines WRITE setHeightHintLines) ;
public:
  KLFLatexEdit(QWidget *parent);
  virtual ~KLFLatexEdit();

  KLFLatexSyntaxHighlighter *syntaxHighlighter() { return mSyntaxHighlighter; }

  /** This function may be used to give a pointer to a KLFDropDataHandler that we will call
   * to open data when we get a paste/drop. If they can open the data, then we consider
   * the data pasted. Otherwise, rely on the QTextEdit built-in functionality.
   *
   * This pointer may also be NULL, in which case we will only rely on QTextEdit built-in
   * functionality. */
  void setDropDataHandler(KLFDropDataHandler *handler) { mDropHandler = handler; }

  /** See sizeHint(). This gets the preferred height of this widget in number of text lines,
   * as set by setHeightHintLints(). */
  inline int heightHintLines() const { return pHeightHintLines; }

  /** The size hint of the widget. If \c heightHintLines() is set to \c -1, this directly
   * calles the superclass function. Otherwise this returns the size in pixels this widget
   * wants to have, given the value of heightHintLines() number of lines in the current font.
   */
  virtual QSize sizeHint() const;


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

  /** See sizeHint(). This sets the preferred height of this widget in number of text lines. */
  void setHeightHintLines(int lines);

  /** Inserts a delimiter \c delim, and brings the cursor \c charsBack characters
   * back. Eg. you can insert \c "\mathrm{}" and bring the cursor 1 space back. */
  void insertDelimiter(const QString& delim, int charsBack = 1);

protected:
  virtual void contextMenuEvent(QContextMenuEvent *event);
  virtual bool canInsertFromMimeData(const QMimeData *source) const;
  virtual void insertFromMimeData(const QMimeData *source);

private slots:
  void slotInsertFromActionSender();

private:
  KLFLatexSyntaxHighlighter *mSyntaxHighlighter;

  /** This is used to open data if needed */
  KLFDropDataHandler *mDropHandler;

  int pHeightHintLines;
};





// ----------------------------------------------


class KLF_EXPORT KLFLatexSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

  Q_PROPERTY(bool highlightEnabled READ highlightEnabled WRITE setHighlightEnabled) ;
  Q_PROPERTY(bool highlightParensOnly READ highlightParensOnly WRITE setHighlightParensOnly) ;
  Q_PROPERTY(bool highlightLonelyParens READ highlightLonelyParens WRITE setHighlightLonelyParens) ;
  Q_PROPERTY(QTextFormat fmtKeyword READ fmtKeyword WRITE setFmtKeyword) ;
  Q_PROPERTY(QTextFormat fmtComment READ fmtComment WRITE setFmtComment) ;
  Q_PROPERTY(QTextFormat fmtParenMatch READ fmtParenMatch WRITE setFmtParenMatch) ;
  Q_PROPERTY(QTextFormat fmtParenMismatch READ fmtParenMismatch WRITE setFmtParenMismatch) ;
  Q_PROPERTY(QTextFormat fmtLonelyParen READ fmtLonelyParen WRITE setFmtLonelyParen) ;

public:
  KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent);
  virtual ~KLFLatexSyntaxHighlighter();

  void setCaretPos(int position);

  virtual void highlightBlock(const QString& text);

  bool highlightEnabled() const { return pConf.enabled; }
  bool highlightParensOnly() const { return pConf.highlightParensOnly; }
  bool highlightLonelyParens() const { return pConf.highlightLonelyParens; }
  QTextCharFormat fmtKeyword() const { return pConf.fmtKeyword; }
  QTextCharFormat fmtComment() const { return pConf.fmtComment; }
  QTextCharFormat fmtParenMatch() const { return pConf.fmtParenMatch; }
  QTextCharFormat fmtParenMismatch() const { return pConf.fmtParenMismatch; }
  QTextCharFormat fmtLonelyParen() const { return pConf.fmtLonelyParen; }

signals:
  void newSymbolTyped(const QString& symbolName);

public slots:
  void refreshAll();

  /** This clears for example the list of already typed symbols. */
  void resetEditing();

  void setHighlightEnabled(bool on);
  void setHighlightParensOnly(bool on);
  void setHighlightLonelyParens(bool on);
  void setFmtKeyword(const QTextFormat& f);
  void setFmtComment(const QTextFormat& f);
  void setFmtParenMatch(const QTextFormat& f);
  void setFmtParenMismatch(const QTextFormat& f);
  void setFmtLonelyParen(const QTextFormat& f);

private:

  QTextEdit *_textedit;

  int _caretpos;

  enum Format { FNormal = 0, FKeyWord, FComment, FParenMatch, FParenMismatch, FLonelyParen };

  struct FormatRule {
    FormatRule(int ps = -1, int l = 0, Format f = FNormal, bool needsfocus = false)
      : pos(ps), len(l), format(f), onlyIfFocus(needsfocus)
    {
      if (len < 0) {
	len = -len; // len is now positive
	pos -= len; // pos is now at beginning of the region
      }
    }
    int pos;
    int len;
    int end() const { return pos + len; }
    Format format;
    bool onlyIfFocus;
  };

  QList<FormatRule> _rulestoapply;

  void parseEverything();

  QTextCharFormat charfmtForFormat(Format f);

  /** symbols that have already been typed and that newSymbolTyped() should not
   * be emitted for any more. */
  QStringList pTypedSymbols;

  struct Conf {
    bool enabled;
    bool highlightParensOnly;
    bool highlightLonelyParens;
    bool matchParenTypes;
    QTextCharFormat fmtKeyword;
    QTextCharFormat fmtComment;
    QTextCharFormat fmtParenMatch;
    QTextCharFormat fmtParenMismatch;
    QTextCharFormat fmtLonelyParen;
  };
  Conf pConf;
};




#endif
