/***************************************************************************
 *   file klfcolorchooser.h
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
/* $Id$ */

#ifndef KLFCOLORCHOOSER_H
#define KLFCOLORCHOOSER_H

#include <QPushButton>
#include <QColor>
#include <QApplication>
#include <QList>
#include <QEvent>
#include <QWidget>
#include <QSpinBox>
#include <QGridLayout>
#include <QPainter>

#include <klfdefs.h>


#define MAX_RECENT_COLORS 128



// ------------------------------------------------------------------------------------


class KLF_EXPORT KLFColorList : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int maxSize READ maxSize WRITE setMaxSize)
public:
  KLFColorList(int maxsize) : QObject(qApp) { _maxsize = maxsize; }

  int maxSize() const { return _maxsize; }

  QList<QColor> list;

signals:
  void listChanged();

public slots:
  void setMaxSize(int maxsize) { _maxsize = maxsize; }
  void addColor(const QColor& color);
  void removeColor(const QColor& color);
  void notifyListChanged() { emit listChanged(); }

private:
  int _maxsize;
};


// ------------------------------------------------------------------------------------


class KLF_EXPORT KLFColorClickSquare : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
public:
  KLFColorClickSquare(QColor color = Qt::white, int size = 16, bool removable = true, QWidget *parent = 0);

  virtual QSize sizeHint() { return QSize(_size, _size); }

  QColor color() const { return _color; }

signals:
  void activated();
  void colorActivated(const QColor& color);
  void wantRemove();
  void wantRemoveColor(const QColor& color);

public slots:
  void setColor(const QColor& col) { _color = col; }
  void activate() {
    emit activated();
    emit colorActivated(_color);
  }

protected:
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);

private:
  QColor _color;
  int _size;
  bool _removable;

private slots:
  void internalWantRemove();
};

// ------------------------------------------------------------------------------------

class KLF_EXPORT KLFGridFlowLayout : public QGridLayout
{
  Q_OBJECT
public:
  KLFGridFlowLayout(int columns, QWidget *parent);
  virtual ~KLFGridFlowLayout() { }

  virtual int ncolumns() const { return _ncols; }

  virtual void insertGridFlowWidget(QWidget *w, Qt::Alignment align = 0);

  void clearAll();

protected:
  QList<QWidget*> mGridFlowWidgets;
  int _ncols;
  int _currow, _curcol;
};


// ------------------------------------------------------------------------------------

class KLF_EXPORT KLFColorComponentsEditorBase
{
public:

protected:
  int valueAFromNewColor(const QColor& color) const;
  int valueBFromNewColor(const QColor& color) const;
  int valueA() const { return valueAFromNewColor(_color); }
  int valueB() const { return valueBFromNewColor(_color); }
  int valueAMax() const { return valueMax(_colorcomponent); }
  int valueBMax() const { return valueMax(_colorcomponent_b); }

  QColor colorFromValues(QColor color_base, int value_a, int value_b = -1);
  /** returns true if color changed */
  bool refreshColorFromInternalValues(int value_a, int value_b = -1);

  static int valueFromNewColor(const QColor& color, const QString& component);
  static int valueMax(const QString& component);

  QColor _color;
  QString _colorcomponent, _colorcomponent_b;
};


// ------------------------------------------------------------------------------------

class KLF_EXPORT KLFColorComponentSpinBox : public QSpinBox, public KLFColorComponentsEditorBase
{
  Q_OBJECT

  Q_PROPERTY(QString colorComponent READ colorComponent WRITE setColorComponent)
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
public:
  KLFColorComponentSpinBox(QWidget *parent);
  virtual ~KLFColorComponentSpinBox() { }

  QString colorComponent() const { return _colorcomponent; }
  QColor color() const { return _color; }

signals:
  void colorChanged(const QColor& color);

public slots:
  void setColorComponent(const QString& component);
  void setColor(const QColor& color);

private slots:
  void internalChanged(int newvalue);

};


// ------------------------------------------------------------------------------------

class KLF_EXPORT KLFColorChooseWidgetPane : public QWidget, public KLFColorComponentsEditorBase
{
  Q_OBJECT
  Q_PROPERTY(QString paneType READ paneType WRITE setPaneType);
  Q_PROPERTY(QColor color READ color WRITE setColor USER true);
public:
  KLFColorChooseWidgetPane(QWidget *parent = 0);
  virtual ~KLFColorChooseWidgetPane() { }

  QString paneType() const { return _colorcomponent + "+" + _colorcomponent_b; }
  QColor color() const { return _color; }

signals:
  void colorChanged(const QColor& color);

public slots:
  void setColor(const QColor& newcolor);
  void setPaneType(const QString& panetype);

protected:
  void paintEvent(QPaintEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);

private:
  QImage _img;
};


// ------------------------------------------------------------------------------------

// this is ugly, but true: include ui_***.h _here_.
// reason: the above widgets are not declared in separate .h files (that would
// pollute the file system ...)
#include <ui_klfcolorchoosewidgetui.h>

class KLF_EXPORT KLFColorChooseWidget : public QWidget, public Ui::KLFColorChooseWidgetUI
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
  Q_PROPERTY(bool alphaEnabled READ alphaEnabled WRITE setAlphaEnabled)
public:
  KLFColorChooseWidget(QWidget *parent = 0);
  virtual ~KLFColorChooseWidget() { }

  QColor color() const { return _color; }

  bool alphaEnabled() const { return _alphaenabled; }

  static void ensureColorListsInstance();
  static void setRecentCustomColors(QList<QColor> recentcolors,
				    QList<QColor> customcolors) {
    ensureColorListsInstance();
    _recentcolors->list = recentcolors;
    _recentcolors->notifyListChanged();
    _customcolors->list = customcolors;
    _customcolors->notifyListChanged();
  }
  static void addRecentColor(const QColor& col);
  static QList<QColor> recentColors() { ensureColorListsInstance(); return _recentcolors->list; }
  static QList<QColor> customColors() { ensureColorListsInstance(); return _customcolors->list; }

signals:
  void colorChanged(const QColor& color);

public slots:
  void setColor(const QColor& color);
  void setAlphaEnabled(bool alpha_enabled);
  void setCurrentToCustomColor();
  void updatePalettes();

  void updatePaletteRecent();
  void updatePaletteStandard();
  void updatePaletteCustom();

protected slots:
  virtual void internalColorChanged(const QColor& newcolor);
  virtual void internalColorNameSelected(QListWidgetItem *item);
  virtual void internalColorNameSet(const QString& colorname);

private:
  QColor _color;
  bool _alphaenabled;

  QList<QObject*> _connectedColorChoosers;

  void fillPalette(KLFColorList *colorlist, QWidget *w);

  static KLFColorList *_recentcolors;
  static KLFColorList *_standardcolors;
  static KLFColorList *_customcolors;
};


// ------------------------------------------------------------------------------------

// this is ugly, but true: include ui_***.h _here_.
#include <ui_klfcolordialogui.h>

class KLF_EXPORT KLFColorDialog : public QDialog, public Ui::KLFColorDialogUI
{
  Q_OBJECT
public:
  KLFColorDialog(QWidget *parent = 0) : QDialog(parent)
  {
    setupUi(this);
    setObjectName("KLFColorDialog");
  }

  static QColor getColor(QColor startwith = Qt::black, bool alphaenabled = true, QWidget *parent = 0);
};




// ------------------------------------------------------------------------------------

class QStyle;

class KLF_EXPORT KLFColorChooser : public QPushButton
{
  Q_OBJECT

  Q_PROPERTY(QSize showSize READ showSize WRITE setShowSize)
  Q_PROPERTY(bool allowDefaultState READ allowDefaultState WRITE setAllowDefaultState)
  Q_PROPERTY(bool autoAddToList READ autoAddToList WRITE setAutoAddToList)
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
  Q_PROPERTY(float pixXAlignFactor READ pixXAlignFactor WRITE setPixXAlignFactor)
  Q_PROPERTY(float pixYAlignFactor READ pixYAlignFactor WRITE setPixYAlignFactor)
  Q_PROPERTY(bool alphaEnabled READ alphaEnabled WRITE setAlphaEnabled)

public:
  KLFColorChooser(QWidget *parent);
  ~KLFColorChooser();

  QSize showSize() const { return _size; }
  bool allowDefaultState() const { return _allowdefaultstate; }
  bool autoAddToList() const { return _autoadd; }
  QColor color() const;
  float pixXAlignFactor() const { return _xalignfactor; }
  float pixYAlignFactor() const { return _yalignfactor; }
  bool alphaEnabled() const { return _alphaenabled; }

  virtual QSize sizeHint() const;

  static void ensureColorListInstance();
  static void setColorList(const QList<QColor>& colorlist);
  static QList<QColor> colorList();

signals:
  void colorChanged(const QColor& newcolor);

public slots:
  void setColor(const QColor& color);
  void setAllowDefaultState(bool allow);
  void setAutoAddToList(bool autoadd) { _autoadd = autoadd; }
  void setShowSize(const QSize& size) { _size = size; }
  void setPixXAlignFactor(float xalignfactor) { _xalignfactor = xalignfactor; }
  void setPixYAlignFactor(float yalignfactor) { _yalignfactor = yalignfactor; }
  void setAlphaEnabled(bool alpha_enabled);
  void setDefaultColor();

  void requestColor();

protected slots:
  void setSenderPropertyColor();
  void _makemenu();

protected:
  void paintEvent(QPaintEvent *event);

private:
  QColor _color;
  QPixmap _pix;

  bool _allowdefaultstate;
  bool _autoadd;
  QSize _size;
  float _xalignfactor, _yalignfactor;

  bool _alphaenabled;

  QMenu *mMenu;

  void _setpix();

  QPixmap colorPixmap(const QColor& color, const QSize& size);

  static KLFColorList *_colorlist;
  static QStyle *mReplaceButtonStyle;
};


#endif
