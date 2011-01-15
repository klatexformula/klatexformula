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

#include <QObject>
#include <QWidget>
#include <QStack>
#include <QTextEdit>
#include <QTextDocumentFragment>
#include <QTextCursor>

#include "klfconfig.h"
#include "klfmainwin.h"

#include "klflatexedit.h"
#include "klflatexedit_p.h"


// declared in klflatexedit_p.h
const char ParenItem::openparenlist[]  = "({[";
const char ParenItem::closeparenlist[] = ")}]";



KLFLatexEdit::KLFLatexEdit(QWidget *parent)
  : QTextEdit(parent), mMainWin(NULL), pHeightHintLines(-1)
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

QSize KLFLatexEdit::sizeHint() const
{
  QSize superSizeHint = QTextEdit::sizeHint();
  if (pHeightHintLines >= 0) {
    return QSize(superSizeHint.width(), 4 + QFontMetrics(font()).height()*pHeightHintLines);
  }
  return superSizeHint;
}

void KLFLatexEdit::setHeightHintLines(int lines)
{
  pHeightHintLines = lines;
  updateGeometry();
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
  static const struct { const char * instext; int charsback; const char * iconsymb; } delimList[] = {
    { "\\frac{}{}", 3, "\\frac{a}{b}" },
    { "\\sqrt{}", 1, "\\sqrt{xyz}" },
    { "\\sqrt[]{}", 3, "\\sqrt[n]{xyz}" },
    { "\\textrm{}", 1, "\\textrm{A}" },
    { "\\textit{}", 1, "\\textit{A}" },
    { "\\textsl{}", 1, "\\textsl{A}" },
    { "\\textbf{}", 1, "\\textbf{A}" },
    { "\\mathrm{}", 1, "\\mathrm{A}" },
    { "\\mathit{}", 1, "\\mathit{A}" },
    { NULL, 0, NULL }
  };

  QMenu *delimmenu = new QMenu(menu);
  int k;
  for (k = 0; delimList[k].instext != NULL; ++k) {
    QAction *a = new QAction(delimmenu);
    a->setText(delimList[k].instext);
    QVariantMap v;
    v["delim"] = QVariant::fromValue<QString>(QLatin1String(delimList[k].instext));
    v["charsBack"] = QVariant::fromValue<int>(delimList[k].charsback);
    a->setData(QVariant(v));
    a->setIcon(KLFLatexSymbolsCache::theCache()->findSymbolPixmap(QLatin1String(delimList[k].iconsymb)));
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

  klfDbg("mMainWin="<<mMainWin<<" did not handle the paste, doing it ourselves.") ;

  // insert the data ourselves
  QTextEdit::insertFromMimeData(data);
}

void KLFLatexEdit::insertDelimiter(const QString& delim, int charsBack)
{
  QTextCursor c1 = textCursor();
  c1.beginEditBlock();
  QString selected = c1.selection().toPlainText();
  QString toinsert = delim;
  if (selected.length())
    toinsert.insert(toinsert.length()-charsBack, selected);
  c1.removeSelectedText();
  c1.insertText(toinsert);
  c1.endEditBlock();

  if (selected.isEmpty())
    c1.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, charsBack);

  setTextCursor(c1);

  setFocus();
}


void KLFLatexEdit::slotInsertFromActionSender()
{
  QObject *obj = sender();
  if (obj == NULL || !obj->inherits("QAction")) {
    qWarning()<<KLF_FUNC_NAME<<": sender object is not a QAction: "<<obj;
    return;
  }
  QVariant v = qobject_cast<QAction*>(obj)->data();
  QVariantMap vdata = v.toMap();
  insertDelimiter(vdata["delim"].toString(), vdata["charsBack"].toInt());
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
  QStack<ParenItem> parens; // the parens that we'll meet
  QList<ParenItem> lonelyparens; // extra lonely parens that we can't close within the text

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
	  strchr(ParenItem::openparenlist, text[i].toAscii()) != NULL) {
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
      if (bsright.indexIn(text.mid(i)) != -1 ||
	  strchr(ParenItem::closeparenlist, text[i].toAscii())) {
	// first determine this closing paren type (char and is-'\left/\right')
	char cp_ch = '!';
	bool cp_left = ( text.mid(i, 6) == "\\right" );
	if (cp_left) {
	  i += 6;
	}
	if (i == text.length()) // ignore a \right with no following character
	  continue;
	if (text.mid(i,2) == "\\}")
	  ++i; // focus on the '}' sign, not the \\ sign
	cp_ch = text[i].toAscii();
	
	ParenItem p;
	if (!parens.empty()) {
	  // first try to match the same paren type, perhaps leaving a lonely unmatched paren between,
	  // eg. in "sin[\theta(1+t]", match both square brackets leaving the paren lonely.
	  // Do this on a copy of the stack, in case we don't find a matching paren
	  QStack<ParenItem> ptrymatch = parens;
	  QList<ParenItem> extralonelyparens;
	  while (ptrymatch.size() && !ParenItem::match(ptrymatch.top().ch, ptrymatch.top().left, cp_ch, cp_left)) {
	    extralonelyparens << ptrymatch.top();
	    ptrymatch.pop();
	  }
	  if (ptrymatch.size()) { // found match
	    parens = ptrymatch;
	    lonelyparens << extralonelyparens;
	    p = parens.top();
	    parens.pop();
	  } else {
	    // No match found, report a lonely paren.
	    int topparenstackpos = 0;
	    if (parens.size()) {
	      topparenstackpos = parens.top().pos + 1;
	    }
	    p = ParenItem(topparenstackpos, false, '!'); // simulate an item
	    if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	      _rulestoapply.append(FormatRule(blockpos+i, 1, FLonelyParen));
	  }
	} else {
	  p = ParenItem(0, false, '!'); // simulate an item
	  if (klfconfig.SyntaxHighlighter.configFlags & HighlightLonelyParen)
	    _rulestoapply.append(FormatRule(blockpos+i, 1, FLonelyParen));
	}
	Format col;
	if (ParenItem::match(p.ch, p.left, cp_ch, cp_left))
	  col = FParenMatch;
	else
	  col = FParenMismatch;

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

  // collect lonely parens list: all unclosed parens should be added to the list of collected
  // unclosed parens.
  while (!parens.empty()) {
    lonelyparens << parens.top();
    parens.pop();
  }

  for (k = 0; k < lonelyparens.size(); ++k) {
    // for each unclosed paren
    ParenItem p = lonelyparens[k];
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
  klfDbg("text is "<<text);

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
