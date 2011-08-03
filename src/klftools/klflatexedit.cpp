/***************************************************************************
 *   file klflatexedit.cpp
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

#include <QObject>
#include <QWidget>
#include <QStack>
#include <QTextEdit>
#include <QTextDocumentFragment>
#include <QTextCursor>
#include <QAction>
#include <QMenu>

#include <klfguiutil.h>


#include "klflatexedit.h"
#include "klflatexedit_p.h"


// declared in klflatexedit_p.h
QStringList KLFLatexSyntaxHighlighter::ParsedBlock::openParenList =
  QStringList() << "(" << "[" << "{" << "\\{";
QStringList KLFLatexSyntaxHighlighter::ParsedBlock::closeParenList =
  QStringList() << ")" << "]" << "}" << "\\}";
QStringList KLFLatexSyntaxHighlighter::ParsedBlock::openParenModifiers =
  QStringList() << "\\left"  << "\\bigl" << "\\Bigl";
QStringList KLFLatexSyntaxHighlighter::ParsedBlock::closeParenModifiers =
  QStringList() << "\\right" << "\\bigr" << "\\Bigr";



KLFLatexEdit::KLFLatexEdit(QWidget *parent)
  : QTextEdit(parent), mDropHandler(NULL), pHeightHintLines(-1)
{
  mSyntaxHighlighter = new KLFLatexSyntaxHighlighter(this, this);

  connect(this, SIGNAL(cursorPositionChanged()),
	  mSyntaxHighlighter, SLOT(refreshAll()));

  setContextMenuPolicy(Qt::DefaultContextMenu);

  setProperty("klfDontChange_font", QVariant(true));
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
  int k;

  if ( ! textCursor().hasSelection() ) {
    // move cursor at that point, but not if we have a selection
    setTextCursor(cursorForPosition(pos));
  }

  QMenu * menu = createStandardContextMenu(mapToGlobal(pos));

  /*
  menu->addSeparator();

  / ** \todo ....make this more flexible ..... ideally integrate into KLFLatexSymbolCache... with XML description,
   *    "symbols" that would be delimiters would have a "display latex" and an "insert latex" with instructions
   *    in if we need to go back spaces, etc.. * /
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
  */

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
  if (mDropHandler != NULL)
    if (mDropHandler->canOpenDropData(data))
      return true; // data can be opened by main window

  // or check if we can insert the data ourselves
  return QTextEdit::canInsertFromMimeData(data);
}

void KLFLatexEdit::insertFromMimeData(const QMimeData *data)
{
  klfDbg("formats: "<<data->formats());
  if (mDropHandler != NULL) {
    int res = mDropHandler->openDropData(data);
    if (res == KLFDropDataHandler::OpenDataOk)
      return; // data was opened by main window
    if (res == KLFDropDataHandler::OpenDataFailed) {
      // NO: eg. for plain text, try again with QTextEdit's paste
      //      // failed to open data, don't insist.
      //      return;
    }
  }

  klfDbg("mDropHandler="<<mDropHandler<<" did not handle the paste, doing it ourselves.") ;

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

  // some reasonable defaults for our config...
  pConf.enabled = true;
  pConf.highlightParensOnly = false;
  pConf.highlightLonelyParens = true;
  pConf.matchParenTypes = true;

  pConf.fmtKeyword.setForeground(QColor(0, 0, 128));
  pConf.fmtComment.setForeground(QColor(180, 0, 0));
  pConf.fmtComment.setFontItalic(true);
  pConf.fmtParenMatch.setBackground(QColor(180, 238, 180));
  pConf.fmtParenMismatch.setBackground(QColor(255, 20, 147));
  pConf.fmtLonelyParen.setForeground(QColor(255, 0, 255));
  pConf.fmtLonelyParen.setFontWeight(QFont::Bold);

  _caretpos = 0;
}

KLFLatexSyntaxHighlighter::~KLFLatexSyntaxHighlighter()
{
}


void KLFLatexSyntaxHighlighter::setHighlightEnabled(bool on)
{
  pConf.enabled = on;
}

void KLFLatexSyntaxHighlighter::setHighlightParensOnly(bool on)
{
  pConf.highlightParensOnly = on;
}
void KLFLatexSyntaxHighlighter::setHighlightLonelyParens(bool on)
{
  pConf.highlightLonelyParens = on;
}
void KLFLatexSyntaxHighlighter::setFmtKeyword(const QTextFormat& f)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  KLF_ASSERT_CONDITION(f.isCharFormat(), "Format "<<f<<" is not a QTextCharFormat.", return ; ) ;
  pConf.fmtKeyword = f.toCharFormat();
}
void KLFLatexSyntaxHighlighter::setFmtComment(const QTextFormat& f)
{
  KLF_ASSERT_CONDITION(f.isCharFormat(), "Format "<<f<<" is not a QTextCharFormat.", return ; ) ;
  pConf.fmtComment = f.toCharFormat();
}
void KLFLatexSyntaxHighlighter::setFmtParenMatch(const QTextFormat& f)
{
  KLF_ASSERT_CONDITION(f.isCharFormat(), "Format "<<f<<" is not a QTextCharFormat.", return ; ) ;
  pConf.fmtParenMatch = f.toCharFormat();
}
void KLFLatexSyntaxHighlighter::setFmtParenMismatch(const QTextFormat& f)
{
  KLF_ASSERT_CONDITION(f.isCharFormat(), "Format "<<f<<" is not a QTextCharFormat.", return ; ) ;
  pConf.fmtParenMismatch = f.toCharFormat();
}
void KLFLatexSyntaxHighlighter::setFmtLonelyParen(const QTextFormat& f)
{
  KLF_ASSERT_CONDITION(f.isCharFormat(), "Format "<<f<<" is not a QTextCharFormat.", return ; ) ;
  pConf.fmtLonelyParen = f.toCharFormat();
}


QList<KLFLatexSyntaxHighlighter::ParsedBlock>
/* */ KLFLatexSyntaxHighlighter::parsedBlocksForPos(int pos, unsigned int filter_mask) const
{
  klfDbg("pos="<<pos<<", filter_mask="<<klfFmtCC("%06x", filter_mask)<<"; total # of blocks="
	 <<pParsedBlocks.size()) ;
  int k;
  QList<ParsedBlock> blocks;
  for (k = 0; k < pParsedBlocks.size(); ++k) {
    klfDbg("testing block #"<<k<<": "<<pParsedBlocks[k]<<"; block/pos+block/len="
	   <<pParsedBlocks[k].pos+pParsedBlocks[k].len<<" compared to pos="<<pos) ;
    if (pParsedBlocks[k].pos <= pos  &&  pos <= pParsedBlocks[k].pos+pParsedBlocks[k].len) {
      if (filter_mask & (1 << pParsedBlocks[k].type)) {
	blocks << pParsedBlocks[k];
	klfDbg("... added #"<<k) ;
      }
    }
  }
  return blocks; // return only the relevant blocks that intersect with position 'pos'
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
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QString text;
  int i = 0;
  int blockpos;
  QList<uint> blocklens; // the length of each block
  QStack<ParenItem> parens; // the parens that we'll meet
  QList<LonelyParenItem> lonelyparens; // extra lonely parens that we can't close within the text

  QTextBlock block = document()->firstBlock();

  QString sopenrx =
    "^(?:("+QStringList(klfListMap(ParsedBlock::openParenModifiers, &QRegExp::escape)).join("|")+")\\s*)?"
    "(" + QStringList(klfListMap(ParsedBlock::openParenList, &QRegExp::escape)).join("|")+")";
  QString scloserx =
    "^(?:("+QStringList(klfListMap(ParsedBlock::closeParenModifiers, &QRegExp::escape)).join("|")+")\\s*)?"
    "(" + QStringList(klfListMap(ParsedBlock::closeParenList, &QRegExp::escape)).join("|")+")";
  klfDbg("open-paren-rx string: "<<sopenrx<<"; close-paren-rx string: "<<scloserx);
  QRegExp rx_open(sopenrx);
  QRegExp rx_close(scloserx);

  // needed to avoid double-parsing of eg. "\\left(" when parsing "\\left(" and then "("
  int lastparenparsingendpos = 0;
  
  _rulestoapply.clear();
  pParsedBlocks.clear();
  int k;
  while (block.isValid()) {
    text = block.text();
    i = 0;
    blockpos = block.position();
    blocklens.append(block.length());

    while (text.length() < block.length()) {
      text += "\n";
    }

    i = 0;
    while ( i < text.length() ) {
      if (text[i] == '%') {
	k = 0;
	while (i+k < text.length() && text[i+k] != '\n')
	  ++k;
	_rulestoapply.append(FormatRule(blockpos+i, k, FComment));
	pParsedBlocks.append(ParsedBlock(ParsedBlock::Comment, blockpos+i, k));
	i += k + 1;
	continue;
      }
      if ( blockpos+i >= lastparenparsingendpos && rx_open.indexIn(text.mid(i)) != -1) {
	ParenItem p;
	p.isopening = true;
	p.parenstr = rx_open.cap(2);
	p.modifier = rx_open.cap(1);
	p.beginpos = blockpos+i;
	p.endpos = blockpos+i+rx_open.matchedLength();
	p.pos = blockpos+i+p.modifier.length();
	p.highlight = (_caretpos == p.caretHoverPos());
	parens.push(p);
	lastparenparsingendpos = p.endpos;
      }
      else if ( blockpos+i >= lastparenparsingendpos && rx_close.indexIn(text.mid(i)) != -1) {
	ParenItem cp;
	cp.isopening = false;
	cp.parenstr = rx_close.cap(2);
	cp.modifier = rx_close.cap(1);
	cp.beginpos = blockpos+i;
	cp.pos = blockpos+i+cp.modifier.length();
	cp.endpos = blockpos+i+rx_close.matchedLength();
	cp.highlight = (_caretpos == cp.caretHoverPos());
	lastparenparsingendpos = cp.endpos;
	
	ParenItem p;
	if (!parens.empty()) {
	  // first try to match the same paren type, perhaps leaving a lonely unmatched paren between,
	  // eg. in "sin[\theta(1+t]", match both square brackets leaving the paren lonely.
	  // Do this on a copy of the stack, in case we don't find a matching paren
	  QStack<ParenItem> ptrymatch = parens;
	  QList<LonelyParenItem> extralonelyparens;
	  while (ptrymatch.size() && !cp.matches(ptrymatch.top())) {
	    extralonelyparens << LonelyParenItem(ptrymatch.top(), cp.beginpos);
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
	      topparenstackpos = parens.top().endpos;
	    }
	    lonelyparens << LonelyParenItem(cp, topparenstackpos);
	    continue; // mismatch will be reported when processing lonely parens
	  }
	} else {
	  lonelyparens << LonelyParenItem(cp, 0);
	  continue; // mismatch will be reported when processing lonely parens
	}
	Format col;
	if (cp.matches(p))
	  col = FParenMatch;
	else
	  col = FParenMismatch;

	// does this rule span multiple paragraphs, and do we need to show it (eg. cursor right after paren)
	if (p.highlight || cp.highlight) {
	  if (pConf.highlightParensOnly) {
	    _rulestoapply.append(FormatRule(p.pos, p.poslength(), col, true));
	    _rulestoapply.append(FormatRule(cp.pos, cp.poslength(), col, true));
	  } else {
	    _rulestoapply.append(FormatRule(p.pos, cp.endpos - p.pos, col, true));
	  }
	}
	ParsedBlock pblk1(ParsedBlock::Paren, p.beginpos, p.beginposlength());
	ParsedBlock pblk2(ParsedBlock::Paren, cp.beginpos, cp.beginposlength());
	pblk1.parenmatch = ((col == FParenMatch) ? ParsedBlock::Matched : ParsedBlock::Mismatched);
	pblk1.parenisopening = true;
	pblk1.parenstr = p.parenstr;
	pblk1.parenmodifier = p.modifier;
	pblk1.parenotherpos = cp.beginpos;
	pblk2.parenmatch = pblk1.parenmatch;
	pblk2.parenisopening = false;
	pblk2.parenstr = cp.parenstr;
	pblk2.parenmodifier = cp.modifier;
	pblk2.parenotherpos = p.beginpos;
	pParsedBlocks.append(pblk1);
	pParsedBlocks.append(pblk2);
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

	QString symbol = text.mid(i-1,k+1); // from i-1, length k+1

	_rulestoapply.append(FormatRule(blockpos+i-1, k+1, FKeyWord));
	ParsedBlock pblk(ParsedBlock::Keyword, blockpos+i-1, k+1);
	pblk.keyword = symbol;
	pParsedBlocks.append(pblk);

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

  int globendpos = lastblock.position()+lastblock.length();

  klfDbg("maybe have some parens left, that are to be shown as lonely? "<<!parens.empty()) ;

  // collect lonely parens list: all unclosed parens should be added to the list of collected
  // unclosed parens.
  while (!parens.empty()) {
    lonelyparens << LonelyParenItem(parens.top(), globendpos);
    parens.pop();
  }

  klfDbg("about to treat "<<lonelyparens.size()<<" lonely parens...") ;

  for (k = 0; k < lonelyparens.size(); ++k) {
    // for each unclosed paren
    LonelyParenItem p = lonelyparens[k];
    int chp = p.caretHoverPos();
    if (chp == _caretpos) {
      if (pConf.highlightParensOnly) {
	_rulestoapply.append(FormatRule(p.pos, p.poslength(), FParenMismatch, true));
      } else {
	// FormatRule will accept a negative length
	_rulestoapply.append(FormatRule(chp, p.unmatchedpos-chp,
					FParenMismatch, true));
      }
    }
    // highlight the lonely paren
    if (pConf.highlightLonelyParens)
      _rulestoapply.append(FormatRule(p.pos, p.poslength(), FLonelyParen));

    ParsedBlock pblk(ParsedBlock::Paren, p.beginpos, p.beginposlength());
    pblk.parenmatch = ParsedBlock::Lonely;
    pblk.parenisopening = p.isopening;
    pblk.parenstr = p.parenstr;
    pblk.parenmodifier = p.modifier;
    pblk.parenotherpos = -1;
    pParsedBlocks.append(pblk);
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
    fmt = pConf.fmtKeyword;
    break;
  case FComment:
    fmt = pConf.fmtComment;
    break;
  case FParenMatch:
    fmt = pConf.fmtParenMatch;
    break;
  case FParenMismatch:
    fmt = pConf.fmtParenMismatch;
    break;
  case FLonelyParen:
    fmt = pConf.fmtLonelyParen;
    break;
  default:
    fmt = QTextCharFormat();
    break;
  };
  return fmt;
}


void KLFLatexSyntaxHighlighter::highlightBlock(const QString& text)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  klfDbg("text is "<<text);

  if ( ! pConf.enabled )
    return; // forget everything about synt highlight if we don't want it.

  QTextBlock block = currentBlock();

  //  printf("\t -- block/position=%d\n", block.position());

  if (block.position() == 0) {
    setCaretPos(_textedit->textCursor().position());
    parseEverything();
  }

  QList<FormatRule> blockfmtrules;

  blockfmtrules.append(FormatRule(0, text.length(), FNormal));

  int k, j;
  for (k = 0; k < _rulestoapply.size(); ++k) {
    int start = _rulestoapply[k].pos - block.position();
    int len = _rulestoapply[k].len;

    if (start < 0) { // the rule starts before current paragraph
      len += start; // "+" because start is negative
      start = 0;
    }
    if (start > text.length())
      continue;
    if (len > text.length() - start)
      len = text.length() - start;

    if (len <= 0)
      continue; // empty rule...
    
    // apply rule
    klfDbg("Applying rule start="<<start<<", len="<<len<<", ...") ;
    blockfmtrules.append(FormatRule(start, len, _rulestoapply[k].format, _rulestoapply[k].onlyIfFocus));
  }

  bool hasfocus = _textedit->hasFocus();

  klfDbg("About to merge text formats... text.length()="<<text.length()) ;
  QVector<QTextCharFormat> charformats;
  charformats.resize(text.length());
  for (k = 0; k < blockfmtrules.size(); ++k) {
    klfDbg("got block-fmt-rule #"<<k<<"; start="<<blockfmtrules[k].pos<<", len="<<blockfmtrules[k].len
	   <<", end="<<blockfmtrules[k].end()) ;
    for (j = blockfmtrules[k].pos; j < blockfmtrules[k].end(); ++j) {
      if ( ! blockfmtrules[k].onlyIfFocus || hasfocus )
	charformats[j].merge(charfmtForFormat(blockfmtrules[k].format));
    }
  }
  klfDbg("About to apply char formats...") ;
  for (j = 0; j < charformats.size(); ++j) {
    setFormat(j, 1, charformats[j]);
  }
  
  return;
}


void KLFLatexSyntaxHighlighter::resetEditing()
{
  pTypedSymbols = QStringList();
}




QDebug operator<<(QDebug str, const KLFLatexSyntaxHighlighter::ParsedBlock& p)
{
  QString stype;
  switch (p.type) {
  case KLFLatexSyntaxHighlighter::ParsedBlock::Normal: stype = "-"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Keyword: stype = "Keyword"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Comment: stype = "Comment"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Paren: stype = "Paren"; break;
  default: stype = "<error>"; break;
  }
  QString smatched;
  switch (p.parenmatch) {
  case KLFLatexSyntaxHighlighter::ParsedBlock::None: smatched = "-"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Matched: smatched = "Matched"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Mismatched: smatched = "Mismatched"; break;
  case KLFLatexSyntaxHighlighter::ParsedBlock::Lonely: smatched = "Lonely"; break;
  default: smatched = "<error>"; break;
  }
  str << "ParsedBlock["<<stype.toLatin1()<<": "<<p.pos<<"+"<<p.len;
  if (p.type == KLFLatexSyntaxHighlighter::ParsedBlock::Keyword) {
    str << ", "<<p.keyword;
  } else if (KLFLatexSyntaxHighlighter::ParsedBlock::Paren) {
    str << ", "<<smatched.toLatin1()<<(p.parenisopening?"(opening)":"(closing)")<<p.parenmodifier<<p.parenstr
	<<" otherpos="<<p.parenotherpos;
  }
  return str << "]";
}
