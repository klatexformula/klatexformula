/***************************************************************************
 *   file klfmainwin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist@bluewin.ch
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

#ifndef KLFMAINWIN_H
#define KLFMAINWIN_H

#include <qvaluelist.h>
#include <qsyntaxhighlighter.h>
#include <qfont.h>

#include <kmainwindow.h>
#include <kpopupmenu.h>
#include <ktextedit.h>

#include <klfbackend.h>

#include <klfdata.h>
#include <klfmainwinui.h>
#include <klfprogerrui.h>


//class KLFMainWinUI;
class KLFHistoryBrowser;
class KLFLatexSymbols;
class KLFStyleManager;


class KLFProgErr : public KLFProgErrUI {
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

};





class KLFLatexSyntaxHighlighter : public QSyntaxHighlighter
{
public:
  KLFLatexSyntaxHighlighter(QTextEdit *textedit);
  virtual ~KLFLatexSyntaxHighlighter();

  void setCaretPos(int para, int pos);

  virtual int highlightParagraph(const QString& text, int endstatelastpara);

  enum { Enabled = 0x01,
	 HighlightParensOnly = 0x02 };
  uint config;

  QColor cKeyword;
  QColor cComment;
  QColor cParenMatch;
  QColor cParenMismatch;

private:

  int _paracount;

  uint _caretpara, _caretpos;

  struct ColorRule {
    ColorRule(int pa = -1, int ps = -1, int l = 0, QColor c = QColor()) : para(pa), pos(ps), len(l), color(c) { }
    int para;
    int pos;
    int len;
    QColor color;
  };

  struct ParenItem {
    ParenItem(int pa = -1, int ps = -1, bool h = false, char c = 0) : para(pa), pos(ps), highlight(h), ch(c) { }
    int para;
    int pos;
    bool highlight;
    char ch;
  };

  QValueList<ColorRule> _rulestoapply;

  void parseEverything();

};






/**
 * KLatexFormula Main Window
 * @author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
*/
class KLFMainWin : public KMainWindow
{
  Q_OBJECT
public:
  KLFMainWin();
  virtual ~KLFMainWin();

  bool eventFilter(QObject *obj, QEvent *event);

  KLFData::KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  QFont txeLatexFont() const { return mMainWidget->font(); }

signals:

  void stylesChanged(); // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly

public slots:

  void slotEvaluate();
  void slotClear();
  void slotHistory(bool showhist);
  void slotHistoryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs);
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();
  void slotQuit();

  void slotDrag();
  void slotCopy();
  void slotSave();

  void slotLoadStyle(int stylenum);
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();

  void refreshSyntaxHighlighting();
  void refreshCaretPosition(int para, int pos);

  void refreshStylePopupMenus();
  void loadStyles();
  void loadHistory();
  void saveStyles();
  void saveHistory();
  void restoreFromHistory(KLFData::KLFHistoryItem h, bool restorestyle);
  void insertSymbol(QString symb);
  void saveSettings();
  void loadSettings();

  void setTxeLatexFont(QFont f) { mMainWidget->txeLatex->setFont(f); }

protected:
  KLFMainWinUI *mMainWidget;
  KLFHistoryBrowser *mHistoryBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;

  KPopupMenu *mStyleMenu;

  KLFLatexSyntaxHighlighter *mHighlighter;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend

  KLFBackend::klfOutput _output; // output from KLFBackend

  KLFData::KLFHistoryList _history;
  KLFData::KLFStyleList _styles;

  QSize _shrinkedsize;
  QSize _expandedsize;

  void saveProperties(KConfig *cfg);
  void readProperties(KConfig *cfg);
};

#endif
