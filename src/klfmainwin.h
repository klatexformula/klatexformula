/***************************************************************************
 *   file klfmainwin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
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

#ifndef KLFMAINWIN_H
#define KLFMAINWIN_H

#include <QList>
#include <QSyntaxHighlighter>
#include <QFont>
#include <QCheckBox>
#include <QMenu>
#include <QTextEdit>
#include <QWidget>

#include <klfbackend.h>

#include <klfconfig.h>
#include <klfdata.h>
#include <klflatexsymbols.h>
#include <ui_klfmainwinui.h>
#include <ui_klfprogerrui.h>


//class KLFMainWinUI;
class KLFLibraryBrowser;
class KLFLatexSymbols;
class KLFStyleManager;



class KLFProgErr : public QDialog, private Ui::KLFProgErrUI {
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

};





class KLFLatexSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT
public:
  KLFLatexSyntaxHighlighter(QTextEdit *textedit, QObject *parent);
  virtual ~KLFLatexSyntaxHighlighter();

  void setCaretPos(int para, int pos);

  virtual void highlightBlock(const QString& text);

  enum { Enabled = 0x01,
	 HighlightParensOnly = 0x02,
	 HighlightLonelyParen = 0x04 };

public slots:
  void refreshAll();

private:

  QTextEdit *_textedit;

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
    ParenItem(int pa = -1, int ps = -1, bool h = false, char c = 0, bool l = false) : para(pa), pos(ps), highlight(h), ch(c), left(l) { }
    int para;
    int pos;
    bool highlight;
    char ch;
    bool left; // if it's \left( instead of (
  };

  QList<ColorRule> _rulestoapply;

  void parseEverything();

};






/**
 * KLatexFormula Main Window
 * @author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
*/
class KLFMainWin : public QWidget, private Ui::KLFMainWinUI
{
  Q_OBJECT
public:
  KLFMainWin();
  virtual ~KLFMainWin();

  bool eventFilter(QObject *obj, QEvent *event);

  KLFData::KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  QFont txtLatexFont() const { return txtLatex->font(); }

signals:

  void stylesChanged(); // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly
  void libraryAllChanged();

public slots:

  void slotEvaluate();
  void slotClear();
  void slotLibrary(bool showhist);
  void slotLibraryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs);
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();

  bool loadNamedStyle(const QString& sty);

  void slotDrag();
  void slotCopy();
  void slotSave();

  void slotLoadStyle(int stylenum);
  void slotLoadStyleAct(); // private : only as slot to an action containing the style # as user data
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();

  void refreshStylePopupMenus();
  void loadStyles();
  void loadLibrary();
  void saveStyles();
  void saveLibrary();
  void restoreFromLibrary(KLFData::KLFLibraryItem h, bool restorestyle);
  void insertSymbol(const KLFLatexSymbol& symbol);
  void saveSettings();
  void loadSettings();

  void setTxtLatexFont(const QFont& f) { txtLatex->setFont(f); }

protected:
  KLFLibraryBrowser *mLibraryBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;

  QMenu *mStyleMenu;

  KLFLatexSyntaxHighlighter *mHighlighter;
  KLFLatexSyntaxHighlighter *mPreambleHighlighter;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend

  KLFBackend::klfOutput _output; // output from KLFBackend

  KLFData::KLFLibrary _library;
  KLFData::KLFLibraryResourceList _libresources;
  KLFData::KLFStyleList _styles;

  QSize _shrinkedsize;
  QSize _expandedsize;

};

#endif
