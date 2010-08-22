/***************************************************************************
 *   file klflatexedit.cpp
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

#include <stack>

#include <QObject>
#include <QWidget>
#include <QTextEdit>

#include "klfconfig.h"
#include "klfmainwin.h"

#include "klflatexedit.h"



KLFLatexEdit::KLFLatexEdit(QWidget *parent)
  : QTextEdit(parent), mMainWin(NULL)
{
  mSyntaxHighlighter = new KLFLatexSyntaxHighlighter(this, this);

  connect(this, SIGNAL(cursorPositionChanged()),
	  mSyntaxHighlighter, SLOT(refreshAll()));

  setContextMenuPolicy(Qt::DefaultContextMenu);
}

KLFLatexEdit::~KLFLatexEdit()
{
}

void KLFLatexEdit::clearLatex()
{
  setLatex("");
  setFocus();
  mSyntaxHighlighter->resetEditing();
}

void KLFLatexEdit::setLatex(const QString& latex)
{
  // don't call setPlainText(); we want to preserve undo history
  QTextCursor cur = textCursor();
  cur.beginEditBlock();
  cur.select(QTextCursor::Document);
  cur.removeSelectedText();
  cur.insertText(latex);
  cur.endEditBlock();
}

void KLFLatexEdit::contextMenuEvent(QContextMenuEvent *event)
{
  QPoint pos = event->pos();

  if ( ! textCursor().hasSelection() ) {
    // move cursor at that point, but not if we have a selection
    setTextCursor(cursorForPosition(pos));
  }

  QMenu * menu = createStandardContextMenu(mapToGlobal(pos));

  menu->addSeparator();

  /** \todo ....make this more flexible ..... ideally integrate into KLFLatexSymbolCache... with XML description,
   *    "symbols" that would be delimiters would have a "display latex" and an "insert latex" with instructions
   *    in if we need to go back spaces, etc.. */
  static QStringList delimList =
    QStringList()<<"\\textrm{}"<<"\\textit{}"<<"\\textsl{}"<<"\\textbf{}"<<"\\mathrm{}"<<"\\mathit{}"<<"\\mathcal{}";
  static QList<int> charsBackList =
    QList<int>() << 1 << 1 << 1 << 1 << 1 << 1 << 1;
  QMenu *delimmenu = new QMenu(menu);
  int k;
  for (k = 0; k < delimList.size() && k < charsBackList.size(); ++k) {
    QAction *a = new QAction(delimmenu);
    a->setText(delimList[k]);
    QVariantMap v;
    v["delim"] = QVariant::fromValue<QString>(delimList[k]);
    v["charsBack"] = QVariant::fromValue<int>(charsBackList[k]);
    a->setData(QVariant(v));
    a->setIcon(KLFLatexSymbolsCache::theCache()->findSymbolPixmap(delimList[k].left(delimList[k].length()-1)+"A}"));
    delimmenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(slotInsertFromActionSender()));
  }

  QAction *delimaction = menu->addAction(tr("Insert Delimiter"));
  delimaction->setMenu(delimmenu);

  QList<QAction*> actionList;
  emit insertContextMenuActions(pos, &actionList);

  if (actionList.size()) {
    menu->addSeparator();
    for (k = 0; k < actionList.size(); ++k) {
      menu->addAction(actionList[k]);
    }
  }
 
  menu->popup(mapToGlobal(pos));
  event->accept();
}


bool KLFLatexEdit::canInsertFromMimeData(const QMimeData *data) const
{
  klfDbg("formats: "<<data->formats());
  if (mMainWin != NULL)
    if (mMainWin->canOpenData(data))
      return true; // data can be opened by main window

  // or check if we can insert the data ourselves
  return QTextEdit::canInsertFromMimeData(data);
}

void KLFLatexEdit::insertFromMimeData(const QMimeData *data)
{
  bool openerfound = false;
  klfDbg("formats: "<<data->formats());
  if (mMainWin != NULL)
    if (mMainWin->openData(data, &openerfound))
      return; // data was opened by main window
  if (openerfound) {
    // failed to open data, don't insist.
    return;
  }

  klfDbg("mMainWin="<<mMainWin<<" did not handle the paste doing it ourselves.") ;

  // insert the data ourselves
  QTextEdit::insertFromMimeData(data);
}


// ------------------------------------


KLFLatexSyntaxHighlighter::KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent)
  : QSyntaxHighlighter(parent) , _textedit(textedit)
{
  setDocument(textedit->document());

  _caretpos = 0;
}

KLFLatexSyntaxHighlighter::~KLFLatexSyntaxHighlighter()
{
}

void KLFLatexSyntaxHighlighter::setCaretPos(int position)
{
  _caretpos = position;
}

void KLFLatexSyntaxHighlighter::refreshAll()
{
  rehighlight();
}

void KLFLatexSyntaxHighlighter::parseEverything()
{
  QString text;
  int i = 0;
  int blockpos;
  QList<uint> blocklens; // the length of each block
  std::stack<ParenItem> parens; // the parens that we'll meet

  QTextBlock block = document()->firstBlock();
  
  _rulestoapply.clear();
  int k;
  while (block.isValid()) {
    text = block.text();
    i = 0;
    blockpos = block.position();
    blocklens.append(block.length());

    while (text.length() < block.length()) {
      text += "\n";
    }

    static QRegExp bsleft("^\\\\left(?!\\w)");
    static QRegExp bsright("^\\\\right(?!\\w)");

    i = 0;
    while ( i < text.length() ) {
      if (text[i] == '%') {
	k = 0;
	while (i+k < text.length() && text[i+k] != '\n')
	  ++k;
	_rulestoapply.append(FormatRule(blockpos+i, k, FComment));
	i += k + 1;
	continue;
      }
      if (bsleft.indexIn(text.mid(i)) != -1 ||
	  text[i] == '{' || text[i] == '(' || text[i] == '[') {
	bool l = (text.mid(i, 5) == "\\left");
	if (l)
	  i += 5;
	if (i == text.length()) // ignore a \left with no following character
	  continue;
	if (text.mid(i,2) == "\\{")
	  ++i; // focus on the '{' sign, not the \\ sign
	parens.push(ParenItem(blockpos+i, (_caretpos == blockpos+i), text[i].toAscii(), l));
	if (i > 0 && text[i-1] == '\\') {
	  --i; // allow the next-next if-block for keywords to highlight this "\\{"
	}
      }
      if (bsright.indexIn(text.mid(i)) != -1 || text[i] == '}' || text[i] == ')' || text[i] == ']') {
	ParenItem p;
	if (!parens.empty()) {
	  p = parens.top();
	  parens.pop();
	} else {
	  p = ParenItem(0, false, '!'); // simulate an item
	  if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	    _rulestoapply.append(FormatRule(blockpos+i, 1, FLonelyParen));
	}
	Format col = FParenMatch;
	bool l = ( text.mid(i, 6) == "\\right" );
	if (l)
	  i += 6;
	if (i == text.length()) // ignore a \right with no following character
	  continue;
	if (text.mid(i,2) == "\\}")
	  ++i; // focus on the '}' sign, not the \\ sign
	if ( (l && text[i] == '.' && p.left) || (l && p.ch == '.' && p.left) ) {
	  // special case with \left( blablabla \right.  or  \left. blablabla \right)
	  col = FParenMatch;
	} else if ((text[i] == '}' && p.ch != '{') ||
	    (text[i] == ')' && p.ch != '(') ||
	    (text[i] == ']' && p.ch != '[') ||
	    (l != p.left) ) {
	  col = FParenMismatch;
	}
	// does this rule span multiple paragraphs, and do we need to show it (eg. cursor right after paren)
	if (p.highlight || (_caretpos == blockpos+i+1)) {
	  if ((klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) == 0) {
	    _rulestoapply.append(FormatRule(p.pos, blockpos+i+1-p.pos, col, true));
	  } else {
	    if (p.ch != '!') // simulated item for first pos
	      _rulestoapply.append(FormatRule(p.pos, 1, col));
	    _rulestoapply.append(FormatRule(blockpos+i, 1, col, true));
	  }
	}
	if (i > 0 && text[i-1] == '\\') {
	  --i; // allow the next if-block for keywords to highlight this "\\}"
	}
      }

      if (text[i] == '\\') { // a keyword ("\symbol")
	++i;
	k = 0;
	if (i >= text.length())
	  continue;
	while (i+k < text.length() && ( (text[i+k] >= 'a' && text[i+k] <= 'z') ||
					(text[i+k] >= 'A' && text[i+k] <= 'Z') ))
	  ++k;
	if (k == 0 && i+1 < text.length())
	  k = 1;
	_rulestoapply.append(FormatRule(blockpos+i-1, k+1, FKeyWord));
	QString symbol = text.mid(i-1,k+1); // from i-1, length k+1
	if (symbol.size() > 1) { // no empty backslash
	  klfDbg("symbol="<<symbol<<" i="<<i<<" k="<<k<<" caretpos="<<_caretpos<<" blockpos="<<blockpos);
	  if ( (_caretpos < blockpos+i ||_caretpos >= blockpos+i+k+1) &&
	       !pTypedSymbols.contains(symbol)) { // not typing symbol
	    klfDbg("newSymbolTyped() about to be emitted for : "<<symbol);
	    emit newSymbolTyped(symbol);
	    pTypedSymbols.append(symbol);
	  }
	}
	i += k;
	continue;
      }

      ++i;
    }

    block = block.next();
  }

  QTextBlock lastblock = document()->lastBlock();

  while ( ! parens.empty() ) {
    // for each unclosed paren left
    ParenItem p = parens.top();
    parens.pop();
    if (_caretpos == p.pos) {
      if ( (klfconfig.SyntaxHighlighter.configFlags & HighlightParensOnly) != 0 )
	_rulestoapply.append(FormatRule(p.pos, 1, FParenMismatch, true));
      else
	_rulestoapply.append(FormatRule(p.pos, lastblock.position()+lastblock.length()-p.pos,
					FParenMismatch, true));
    } else { // not on caret positions
      if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	_rulestoapply.append(FormatRule(p.pos, 1, FLonelyParen));
    }
  }
}

QTextCharFormat KLFLatexSyntaxHighlighter::charfmtForFormat(Format f)
{
  QTextCharFormat fmt;
  switch (f) {
  case FNormal:
    fmt = QTextCharFormat();
    break;
  case FKeyWord:
    fmt = klfconfig.SyntaxHighlighter.fmtKeyword;
    break;
  case FComment:
    fmt = klfconfig.SyntaxHighlighter.fmtComment;
    break;
  case FParenMatch:
    fmt = klfconfig.SyntaxHighlighter.fmtParenMatch;
    break;
  case FParenMismatch:
    fmt = klfconfig.SyntaxHighlighter.fmtParenMismatch;
    break;
  case FLonelyParen:
    fmt = klfconfig.SyntaxHighlighter.fmtLonelyParen;
    break;
  default:
    fmt = QTextCharFormat();
    break;
  };
  return fmt;
}


void KLFLatexSyntaxHighlighter::highlightBlock(const QString& text)
{
  //  printf("-- HIGHLIGHTBLOCK --:\n%s\n", (const char*)text.toLocal8Bit());

  if ( ( klfconfig.SyntaxHighlighter.configFlags & Enabled ) == 0)
    return; // forget everything about synt highlight if we don't want it.

  QTextBlock block = currentBlock();

  //  printf("\t -- block/position=%d\n", block.position());

  if (block.position() == 0) {
    setCaretPos(_textedit->textCursor().position());
    parseEverything();
  }

  QList<FormatRule> blockfmtrules;
  QVector<QTextCharFormat> charformats;

  charformats.resize(text.length());

  blockfmtrules.append(FormatRule(0, text.length(), FNormal));

  int k, j;
  for (k = 0; k < _rulestoapply.size(); ++k) {
    int start = _rulestoapply[k].pos - block.position();
    int len = _rulestoapply[k].len;
    
    if (start < 0) {
      len += start; // +, start being negative
      start = 0;
    }
    if (start > text.length())
      continue;
    if (len > text.length() - start)
      len = text.length() - start;
    // apply rule
    blockfmtrules.append(FormatRule(start, len, _rulestoapply[k].format, _rulestoapply[k].onlyIfFocus));
  }

  bool hasfocus = _textedit->hasFocus();
  for (k = 0; k < blockfmtrules.size(); ++k) {
    for (j = blockfmtrules[k].pos; j < blockfmtrules[k].end(); ++j) {
      if ( ! blockfmtrules[k].onlyIfFocus || hasfocus )
	charformats[j].merge(charfmtForFormat(blockfmtrules[k].format));
    }
  }
  for (j = 0; j < charformats.size(); ++j) {
    setFormat(j, 1, charformats[j]);
  }
  
  return;
}


void KLFLatexSyntaxHighlighter::resetEditing()
{
  pTypedSymbols = QStringList();
}
