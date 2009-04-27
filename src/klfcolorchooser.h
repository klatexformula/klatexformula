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



class KLFColorList : public QObject
{
  Q_OBJECT
public:
  KLFColorList() : QObject(qApp) { }

  QList<QColor> list;

signals:
  void listChanged();

public slots:
  void addColor(const QColor& color);
  void notifyListChanged() { emit listChanged(); }
};



// ------------------------------------------------------------------------------------


class KLFColorClickSquare : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
public:
  KLFColorClickSquare(QColor color = Qt::white, int size = 16, QWidget *parent = 0)
    : QWidget(parent), _color(color), _size(size)
  {
    setFixedSize(_size, _size);
  }

  virtual QSize sizeHint() { return QSize(_size, _size); }

  QColor color() const { return _color; }

signals:
  void activated();
  void colorActivated(const QColor& color);

public slots:
  void setColor(const QColor& col) { _color = col; }
  void activate() {
    emit activated();
    emit colorActivated(_color);
  }

protected:
  void paintEvent(QPaintEvent */*event*/)
  {
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), QBrush(_color));
  }
  void mousePressEvent(QMouseEvent */*event*/)
  {
    activate();
  }

private:
  QColor _color;
  int _size;
};

// ------------------------------------------------------------------------------------

class KLFGridFlowLayout : public QGridLayout
{
  Q_OBJECT
public:
  KLFGridFlowLayout(int columns, QWidget *parent)
    : QGridLayout(parent), _ncols(columns),
      _currow(0), _curcol(0)
  {
    addItem(new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, _ncols);
  }
  virtual ~KLFGridFlowLayout() { }

  virtual int ncolumns() const { return _ncols; }

  virtual void insertGridFlowWidget(QWidget *w, Qt::Alignment align = 0)
  {
    mGridFlowWidgets.append(w);
    QGridLayout::addWidget(w, _currow, _curcol, align);
    _curcol++;
    if (_curcol >= _ncols) {
      _curcol = 0;
      _currow++;
    }
  }

  void clearAll()
  {
    int k;
    for (k = 0; k < mGridFlowWidgets.size(); ++k) {
      delete mGridFlowWidgets[k];
    }
    mGridFlowWidgets.clear();
    _currow = _curcol = 0;
  }

protected:
  QList<QWidget*> mGridFlowWidgets;
  int _ncols;
  int _currow, _curcol;
};


// ------------------------------------------------------------------------------------

class KLFColorComponentsEditorBase
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

class KLFColorComponentSpinBox : public QSpinBox, public KLFColorComponentsEditorBase
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

class KLFColorChooseWidgetPane : public QWidget, public KLFColorComponentsEditorBase
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

#include <ui_klfcolorchoosewidgetui.h>

class KLFColorChooseWidget : public QWidget, public Ui::KLFColorChooseWidgetUI
{
  Q_OBJECT

  Q_PROPERTY(QColor color READ color WRITE setColor USER true)

public:
  KLFColorChooseWidget(QWidget *parent = 0);
  virtual ~KLFColorChooseWidget() { }

  QColor color() const { return _color; }

  static void setRecentCustomColors(QList<QColor> recentcolors,
				    QList<QColor> customcolors) {
    _recentcolors = recentcolors;
    _customcolors = customcolors;
  }
  static void addRecentColor(const QColor& col) { _recentcolors.append(col); }
  static QList<QColor> recentColors() { return _recentcolors; }
  static QList<QColor> customColors() { return _customcolors; }

signals:
  void colorChanged(const QColor& color);

public slots:
  void setColor(const QColor& color);
  void setCurrentToCustomColor();
  void updatePalettes();

protected slots:
  virtual void internalColorChanged(const QColor& newcolor);
  virtual void internalColorNameSelected(QListWidgetItem *item);
  virtual void internalColorNameSet(const QString& colorname);

private:
  QColor _color;

  QList<QObject*> _connectedColorChoosers;

  void fillPalette(QList<QColor> colorlist, QWidget *w);

  static QList<QColor> _recentcolors;
  static QList<QColor> _standardcolors;
  static QList<QColor> _customcolors;
};


// ------------------------------------------------------------------------------------

#include <ui_klfcolordialogui.h>

class KLFColorDialog : public QDialog, public Ui::KLFColorDialogUI
{
  Q_OBJECT
public:
  KLFColorDialog(QWidget *parent = 0) : QDialog(parent)
  {
    setupUi(this);
  }

  static QColor getColor(QColor startwith = Qt::black, QWidget *parent = 0)
  {
    KLFColorDialog dlg(parent);
    dlg.mColorChooseWidget->setColor(startwith);
    int r = dlg.exec();
    if ( r != QDialog::Accepted )
      return QColor();
    QColor color = dlg.mColorChooseWidget->color();
    KLFColorChooseWidget::addRecentColor(color);
    return color;
  }
};

// ------------------------------------------------------------------------------------

class KLFColorChooser : public QPushButton
{
  Q_OBJECT

  Q_PROPERTY(QSize showSize READ showSize WRITE setShowSize)
  Q_PROPERTY(bool allowDefaultState READ allowDefaultState WRITE setAllowDefaultState)
  Q_PROPERTY(bool autoAddToList READ autoAddToList WRITE setAutoAddToList)
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
  Q_PROPERTY(float pixXAlignFactor READ pixXAlignFactor WRITE setPixXAlignFactor)
  Q_PROPERTY(float pixYAlignFactor READ pixYAlignFactor WRITE setPixYAlignFactor)


public:
  KLFColorChooser(QWidget *parent);
  ~KLFColorChooser();

  QSize showSize() const { return _size; }
  bool allowDefaultState() const { return _allowdefaultstate; }
  bool autoAddToList() const { return _autoadd; }
  QColor color() const;
  float pixXAlignFactor() const { return _xalignfactor; }
  float pixYAlignFactor() const { return _yalignfactor; }

  virtual QSize sizeHint() const;

  static void ensureColorListInstance();
  static void setColorList(const QList<QColor>& colorlist);
  static QList<QColor> colorList();

public slots:
  void setColor(const QColor& color);
  void setAllowDefaultState(bool allow);
  void setAutoAddToList(bool autoadd) { _autoadd = autoadd; }
  void setShowSize(const QSize& size) { _size = size; }
  void setPixXAlignFactor(float xalignfactor) { _xalignfactor = xalignfactor; }
  void setPixYAlignFactor(float yalignfactor) { _yalignfactor = yalignfactor; }
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

  QMenu *mMenu;

  void _setpix();

  QPixmap colorPixmap(const QColor& color, const QSize& size);

  static KLFColorList *_colorlist;
};


#endif
