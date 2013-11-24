/***************************************************************************
 *   file klfcolorchooser.cpp
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

#include <stdio.h>

#include <QAction>
#include <QMenu>
#include <QDebug>
#include <QStylePainter>
#include <QColorDialog>
#include <QPaintEvent>
#include <QStyle>
#include <QPlastiqueStyle>
#include <QStyleOptionButton>
#include <QRegExp>

#include <klfdefs.h>
#include "klfflowlayout.h"
#include "klfguiutil.h"
#include "klfrelativefont.h"
#include "klfcolorchooser.h"
#include "klfcolorchooser_p.h"

#include <ui_klfcolorchoosewidget.h>
#include <ui_klfcolordialog.h>



// -------------------------------------------------------------------


KLFColorDialog::KLFColorDialog(QWidget *parent) : QDialog(parent)
{
  u = new Ui::KLFColorDialog;
  u->setupUi(this);
  setObjectName("KLFColorDialog");
}
KLFColorDialog::~KLFColorDialog()
{
  delete u;
}

KLFColorChooseWidget *KLFColorDialog::colorChooseWidget()
{
  return u->mColorChooseWidget;
}

QColor KLFColorDialog::getColor(QColor startwith, bool alphaenabled, QWidget *parent)
{
  KLFColorDialog dlg(parent);
  dlg.u->mColorChooseWidget->setAlphaEnabled(alphaenabled);
  dlg.u->mColorChooseWidget->setColor(startwith);
  int r = dlg.exec();
  if ( r != QDialog::Accepted )
    return QColor();
  QColor color = dlg.u->mColorChooseWidget->color();
  return color;
}

QColor KLFColorDialog::color() const
{
  return u->mColorChooseWidget->color();
}
void KLFColorDialog::setColor(const QColor& color)
{
  u->mColorChooseWidget->setColor(color);
}
void KLFColorDialog::slotAccepted()
{
  KLFColorChooseWidget::addRecentColor(color());
}

// -------------------------------------------------------------------

KLFColorClickSquare::KLFColorClickSquare(QColor color, int size, bool removable, QWidget *parent)
  : QWidget(parent), _color(color), _removable(removable)
{
  initwidget();
  setSqSize(size);
}
KLFColorClickSquare::KLFColorClickSquare(QWidget *parent)
  : QWidget(parent), _color(Qt::white), _removable(false)
{
  initwidget();
  setSqSize(16);
}
void KLFColorClickSquare::initwidget()
{
  setFocusPolicy(Qt::StrongFocus);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  //  setAutoFillBackground(true);
  //  update();
}

void KLFColorClickSquare::setSqSize(int sz)
{
  if (_size == sz)
    return;

  _size = sz;
  setFixedSize(_size, _size);
}

void KLFColorClickSquare::setRemovable(bool removable)
{
  _removable = removable;
}

void KLFColorClickSquare::paintEvent(QPaintEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  Q_UNUSED(event) ;
  klfDbg("event->rect="<<event->rect()) ;
  {
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), QBrush(_color));
  }
  if (hasFocus()) {
    QStylePainter p(this);
    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.backgroundColor = QColor(0,0,0,0);
    p.drawPrimitive(QStyle::PE_FrameFocusRect, option);
  }
}
void KLFColorClickSquare::resizeEvent(QResizeEvent *event)
{
  Q_UNUSED(event) ;
}

void KLFColorClickSquare::mousePressEvent(QMouseEvent */*event*/)
{
  activate();
}
void KLFColorClickSquare::keyPressEvent(QKeyEvent *kev)
{
  if (kev->key() == Qt::Key_Space) {
    activate();
  }
  return QWidget::keyPressEvent(kev);
}
void KLFColorClickSquare::contextMenuEvent(QContextMenuEvent *event)
{
  if (_removable) {
    QMenu *menu = new QMenu(this);
    menu->addAction("Remove", this, SLOT(internalWantRemove()));
    menu->popup(event->globalPos());
  }
}
void KLFColorClickSquare::internalWantRemove()
{
  emit wantRemove();
  emit wantRemoveColor(_color);
}

// -------------------------------------------------------------------

KLFColorChooseWidgetPane::KLFColorChooseWidgetPane(QWidget *parent)
  : QWidget(parent), _img()
{
  setPaneType("red+fix");
  _color = Qt::black;
}

QSize KLFColorChooseWidgetPane::sizeHint() const
{
  return QSize((_colorcomponent == "fix") ? 16 : 50, (_colorcomponent_b == "fix") ? 16 : 50);
}
QSize KLFColorChooseWidgetPane::minimumSizeHint() const
{
  return QSize(16, 16);
}

void KLFColorChooseWidgetPane::setColor(const QColor& newcolor)
{
  if (_color == newcolor)
    return;

  _color = newcolor;
  update();
  emit colorChanged(_color);
}
void KLFColorChooseWidgetPane::setPaneType(const QString& panetype)
{
  static QStringList okvals =
    QStringList() << "hue"<<"sat"<<"val"<<"red"<<"green"<<"blue"<<"alpha"<<"fix";

  QStringList strlist = panetype.split("+");
  if (strlist.size() != 2) {
    qWarning()<<KLF_FUNC_NAME<<": expected a pane-type string \"<pane1type>+<pane2type>\"!";
    return;
  }
  _colorcomponent = strlist[0].toLower();
  _colorcomponent_b = strlist[1].toLower();
  if (!okvals.contains(_colorcomponent))
    _colorcomponent = "fix";
  if (!okvals.contains(_colorcomponent_b))
    _colorcomponent_b = "fix";

  if (_colorcomponent == "fix" && _colorcomponent_b == "fix")
    setFocusPolicy(Qt::NoFocus);
  else
    setFocusPolicy(Qt::WheelFocus);
}
void KLFColorChooseWidgetPane::paintEvent(QPaintEvent */*e*/)
{
  QStylePainter p(this);
  // background: a checker grid to distinguish transparency
  p.fillRect(0,0,width(),height(), QBrush(QPixmap(":/pics/checker.png")));
  // then prepare an image for our gradients
  int x;
  int y;
  _img = QImage(width(), height(), QImage::Format_ARGB32);
  double xfac = (double)valueAMax() / (_img.width()-1);
  double yfac = (double)valueBMax() / (_img.height()-1);
  for (x = 0; x < _img.width(); ++x) {
    for (y = 0; y < _img.height(); ++y) {
      _img.setPixel(x, _img.height()-y-1, colorFromValues(_color, (int)(xfac*x), (int)(yfac*y)).rgba());
    }
  }
  p.drawImage(0, 0, _img);
  // draw crosshairs
  QColor hairscol = qGray(_color.rgb()) > 80 ? QColor(0,0,0,180) : QColor(255,255,255,180);
  if ( _colorcomponent != "fix" ) {
    p.setPen(QPen(hairscol, 1.f, Qt::DotLine));
    x = (int)(valueA()/xfac);
    if (x < 0) x = 0; if (x >= width()) x = width()-1;
    p.drawLine(x, 0, x, height());
  }
  if ( _colorcomponent_b != "fix" ) {
    p.setPen(QPen(hairscol, 1.f, Qt::DotLine));
    y = (int)(valueB()/yfac);
    if (y < 0) y = 0; if (y >= height()) y = height()-1;
    p.drawLine(0, height()-y-1, width(), height()-y-1);
  }
  // draw a focus rectangle if we have focus
  if (hasFocus()) {
    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.backgroundColor = QColor(0,0,0,0);
    p.drawPrimitive(QStyle::PE_FrameFocusRect, option);
  }
}
void KLFColorChooseWidgetPane::mousePressEvent(QMouseEvent *e)
{
  double xfac = (double)valueAMax() / (_img.width()-1);
  double yfac = (double)valueBMax() / (_img.height()-1);
  int x = e->pos().x();
  int y = height() - e->pos().y() - 1;

  setColor(colorFromValues(_color, (int)(x*xfac), (int)(y*yfac)));
}
void KLFColorChooseWidgetPane::mouseMoveEvent(QMouseEvent *e)
{
  double xfac = (double)valueAMax() / (_img.width()-1);
  double yfac = (double)valueBMax() / (_img.height()-1);
  int x = e->pos().x();
  int y = height() - e->pos().y() - 1;
  if (x < 0) x = 0; if (x >= width()) x = width()-1;
  if (y < 0) y = 0; if (y >= height()) y = height()-1;

  setColor(colorFromValues(_color, (int)(x*xfac), (int)(y*yfac)));
}
void KLFColorChooseWidgetPane::wheelEvent(QWheelEvent *e)
{
  double step = - 7.5 * e->delta() / 120;

  if (e->modifiers() == Qt::ShiftModifier)
    step = step / 5.0;
  if (e->modifiers() == Qt::ControlModifier)
    step = step * 2.5;

  // isA: TRUE if we are modifying component A, if FALSE then modifying component B
  bool isA =  (e->orientation() == Qt::Horizontal);
  if (isA && _colorcomponent=="fix")
    isA = false;
  if (!isA && _colorcomponent_b=="fix")
    isA = true;
  if (isA) {
    // the first component
    int x = (int)(valueA()+step);
    if (x < 0) x = 0;
    if (x > valueAMax()) x = valueAMax();
    setColor(colorFromValues(_color, x, valueB()));
  } else {
    int x = (int)(valueB() - step);
    if (x < 0) x = 0;
    if (x > valueBMax()) x = valueBMax();
    setColor(colorFromValues(_color, valueA(), x));
  }
  e->accept();
}
void KLFColorChooseWidgetPane::keyPressEvent(QKeyEvent *e)
{
  const int dir_step = 5;
  double xstep = 0;
  double ystep = 0;

  if (e->key() == Qt::Key_Left)
    xstep -= dir_step;
  if (e->key() == Qt::Key_Right)
    xstep += dir_step;
  if (e->key() == Qt::Key_Up)
    ystep += dir_step;
  if (e->key() == Qt::Key_Down)
    ystep -= dir_step;
  if (e->key() == Qt::Key_Home)
    xstep = -10000;
  if (e->key() == Qt::Key_End)
    xstep = 10000;
  if (e->key() == Qt::Key_PageUp)
    ystep = 10000;
  if (e->key() == Qt::Key_PageDown)
    ystep = -10000;

  // if a component is set to 'fix', add the deltas to the other component...
  if (_colorcomponent == "fix") {
    ystep += xstep;
    xstep = 0;
  } else if (_colorcomponent_b == "fix") {
    xstep += ystep;
    ystep = 0;
  }

  if (e->modifiers() == Qt::ShiftModifier) {
    xstep = xstep / 5; ystep = ystep / 5;
  }
  if (e->modifiers() == Qt::ControlModifier) {
    xstep = xstep * 2.5; ystep = ystep * 2.5;
  }

  int x = (int)(valueA() + xstep);
  int y = (int)(valueB() + ystep);
  if (x < 0) x = 0;
  if (x > valueAMax()) x = valueAMax();
  if (y < 0) y = 0;
  if (y > valueBMax()) y = valueBMax();

  setColor(colorFromValues(_color, x, y));
}


// -------------------------------------------------------------------


KLFGridFlowLayout::KLFGridFlowLayout(int columns, QWidget *parent)
  : QGridLayout(parent), _ncols(columns),
    _currow(0), _curcol(0)
{
  addItem(new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, _ncols);
}
void KLFGridFlowLayout::insertGridFlowWidget(QWidget *w, Qt::Alignment align)
{
  mGridFlowWidgets.append(w);
  QGridLayout::addWidget(w, _currow, _curcol, align);
  _curcol++;
  if (_curcol >= _ncols) {
    _curcol = 0;
    _currow++;
  }
}
void KLFGridFlowLayout::clearAll()
{
  int k;
  for (k = 0; k < mGridFlowWidgets.size(); ++k) {
    // because KLFColorClickSquare::wantRemoveColor() can call this by a chain of
    // signal/slots; and we shouldn't delete an object inside one of its handlers
    //delete mGridFlowWidgets[k];
    mGridFlowWidgets[k]->deleteLater();
  }
  mGridFlowWidgets.clear();
  _currow = _curcol = 0;
}


// -------------------------------------------------------------------


int KLFColorComponentsEditorBase::valueAFromNewColor(const QColor& color) const
{
  return valueFromNewColor(color, _colorcomponent);
}
int KLFColorComponentsEditorBase::valueBFromNewColor(const QColor& color) const
{
  return valueFromNewColor(color, _colorcomponent_b);
}
int KLFColorComponentsEditorBase::valueFromNewColor(const QColor& color, const QString& component)
{
  int value = -1;
  if (component == "hue") {
    value = color.hue();
  } else if (component == "sat") {
    value = color.saturation();
  } else if (component == "val") {
    value = color.value();
  } else if (component == "red") {
    value = color.red();
  } else if (component == "green") {
    value = color.green();
  } else if (component == "blue") {
    value = color.blue();
  } else if (component == "alpha") {
    value = color.alpha();
  } else if (component == "fix" || component.isEmpty()) {
    value = -1;
  } else {
    qWarning("Unknown color component property : %s", component.toLocal8Bit().constData());
  }
  return value;
}

int KLFColorComponentsEditorBase::valueMax(const QString& component)
{
  if (component == "hue")
    return 359;
  else if (component == "sat" || component == "val" ||
	   component == "red" || component == "green" ||
	   component == "blue" || component == "alpha")
    return 255;
  else if (component == "fix" || component.isEmpty())
    return -1;

  qWarning("Unknown color component property : %s", component.toLocal8Bit().constData());
  return -1;
}

QColor KLFColorComponentsEditorBase::colorFromValues(QColor base, int a, int b)
{
  QColor col = base;
  /*  printf("colorFromValues(%s/alpha=%d, %d, %d): My components:(%s+%s);\n", qPrintable(col.name()),
      col.alpha(), a, b, qPrintable(_colorcomponent), qPrintable(_colorcomponent_b)); */
  if (_colorcomponent == "hue") {
    col.setHsv(a, col.saturation(), col.value());
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "sat") {
    col.setHsv(col.hue(), a, col.value());
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "val") {
    col.setHsv(col.hue(), col.saturation(), a);
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "red") {
    col.setRgb(a, col.green(), col.blue());
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "green") {
    col.setRgb(col.red(), a, col.blue());
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "blue") {
    col.setRgb(col.red(), col.green(), a);
    col.setAlpha(base.alpha());
  } else if (_colorcomponent == "alpha") {
    col.setAlpha(a);
  } else if (_colorcomponent == "fix") {
    // no change to col
  } else {
    qWarning("Unknown color component property : %s", _colorcomponent.toLocal8Bit().constData());
  }
  QColor base2 = col;
  //  printf("\tnew color is (%s/alpha=%d);\n", qPrintable(col.name()), col.alpha());
  if ( ! _colorcomponent_b.isEmpty() && _colorcomponent_b != "fix" ) {
    //    printf("\twe have a second component\n");
    if (_colorcomponent_b == "hue") {
      col.setHsv(b, col.saturation(), col.value());
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "sat") {
      col.setHsv(col.hue(), b, col.value());
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "val") {
      col.setHsv(col.hue(), col.saturation(), b);
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "red") {
      col.setRgb(b, col.green(), col.blue());
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "green") {
      col.setRgb(col.red(), b, col.blue());
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "blue") {
      col.setRgb(col.red(), col.blue(), b);
      col.setAlpha(base2.alpha());
    } else if (_colorcomponent_b == "alpha") {
      col.setAlpha(b);
    } else {
      qWarning("Unknown color component property : %s", _colorcomponent_b.toLocal8Bit().constData());
    }
  }
  //  printf("\tand color is finally %s/alpha=%d\n", qPrintable(col.name()), col.alpha());
  return col;
}
bool KLFColorComponentsEditorBase::refreshColorFromInternalValues(int a, int b)
{
  QColor oldcolor = _color;
  _color = colorFromValues(_color, a, b);
  /*  printf("My components:(%s+%s); New color is %s/alpha=%d\n", _colorcomponent.toLocal8Bit().constData(),
      _colorcomponent_b.toLocal8Bit().constData(),  _color.name().toLocal8Bit().constData(), _color.alpha()); */
  if ( oldcolor != _color )
    return true;
  return false;
}


// -------------------------------------------------------------------


KLFColorComponentSpinBox::KLFColorComponentSpinBox(QWidget *parent)
  : QSpinBox(parent)
{
  _color = Qt::black;

  setColorComponent("hue");
  setColor(_color);

  connect(this, SIGNAL(valueChanged(int)), this, SLOT(internalChanged(int)));

  setValue(valueAFromNewColor(_color));
}

void KLFColorComponentSpinBox::setColorComponent(const QString& comp)
{
  _colorcomponent = comp.toLower();
  setMinimum(0);
  setMaximum(valueAMax());
}

void KLFColorComponentSpinBox::internalChanged(int newvalue)
{
  if ( refreshColorFromInternalValues(newvalue) )
    emit colorChanged(_color);
}

void KLFColorComponentSpinBox::setColor(const QColor& color)
{
  if (_color == color)
    return;
  int value = valueAFromNewColor(color);
  /*  printf("My components:(%s+%s); setColor(%s/alpha=%d); new value = %d\n",
      _colorcomponent.toLocal8Bit().constData(), _colorcomponent_b.toLocal8Bit().constData(),
      color.name().toLocal8Bit().constData(), color.alpha(), value); */
  _color = color;
  setValue(value); // will emit QSpinBox::valueChanged() --> internalChanged() --> colorChanged()
}


// -------------------------------------------------------------------


KLFColorList * KLFColorChooseWidget::_recentcolors = 0;
KLFColorList * KLFColorChooseWidget::_standardcolors = 0;
KLFColorList * KLFColorChooseWidget::_customcolors = 0;

// static
void KLFColorChooseWidget::setRecentCustomColors(QList<QColor> recentcolors, QList<QColor> customcolors)
{
  ensureColorListsInstance();
  _recentcolors->list = recentcolors;
  _recentcolors->notifyListChanged();
  _customcolors->list = customcolors;
  _customcolors->notifyListChanged();
}
// static
QList<QColor> KLFColorChooseWidget::recentColors()
{
  ensureColorListsInstance();
  return _recentcolors->list;
}
// static
QList<QColor> KLFColorChooseWidget::customColors()
{
  ensureColorListsInstance();
  return _customcolors->list;
}


KLFColorChooseWidget::KLFColorChooseWidget(QWidget *parent)
  : QWidget(parent)
{
  u = new Ui::KLFColorChooseWidget;
  u->setupUi(this);
  setObjectName("KLFColorChooseWidget");

  _alphaenabled = true;

  ensureColorListsInstance();

  if (_standardcolors->list.size() == 0) {
    // add a few standard colors.
    QList<QRgb> rgbs;
    // inspired from the "Forty Colors" Palette in KDE3 color dialog
    rgbs << 0x000000 << 0x303030 << 0x585858 << 0x808080 << 0xa0a0a0 << 0xc3c3c3
	 << 0xdcdcdc << 0xffffff << 0x400000 << 0x800000 << 0xc00000 << 0xff0000
	 << 0xffc0c0 << 0x004000 << 0x008000 << 0x00c000 << 0x00ff00 << 0xc0ffc0
	 << 0x000040 << 0x000080 << 0x0000c0 << 0x0000ff << 0xc0c0ff << 0x404000
	 << 0x808000 << 0xc0c000 << 0xffff00 << 0xffffc0 << 0x004040 << 0x008080
	 << 0x00c0c0 << 0x00ffff << 0xc0ffff << 0x400040 << 0x800080 << 0xc000c0
	 << 0xff00ff << 0xffc0ff << 0xc05800 << 0xff8000 << 0xffa858 << 0xffdca8 ;
    for (int k = 0; k < rgbs.size(); ++k)
      _standardcolors->list.append(QColor(QRgb(rgbs[k])));
  }

  _connectedColorChoosers.append(u->mDisplayColor);
  _connectedColorChoosers.append(u->mHueSatPane);
  _connectedColorChoosers.append(u->mValPane);
  _connectedColorChoosers.append(u->mAlphaPane);
  _connectedColorChoosers.append(u->mColorTriangle);
  _connectedColorChoosers.append(u->mHueSlider);
  _connectedColorChoosers.append(u->mSatSlider);
  _connectedColorChoosers.append(u->mValSlider);
  _connectedColorChoosers.append(u->mRedSlider);
  _connectedColorChoosers.append(u->mGreenSlider);
  _connectedColorChoosers.append(u->mBlueSlider);
  _connectedColorChoosers.append(u->mAlphaSlider);
  _connectedColorChoosers.append(u->spnHue);
  _connectedColorChoosers.append(u->spnSat);
  _connectedColorChoosers.append(u->spnVal);
  _connectedColorChoosers.append(u->spnRed);
  _connectedColorChoosers.append(u->spnGreen);
  _connectedColorChoosers.append(u->spnBlue);
  _connectedColorChoosers.append(u->spnAlpha);

  /*  KLFGridFlowLayout *lytRecent = new KLFGridFlowLayout(12, u->mRecentColorsPalette);
      lytRecent->setSpacing(2);
      //  lytRecent->setSizeConstraint(QLayout::SetMinAndMaxSize);
      KLFGridFlowLayout *lytStandard = new KLFGridFlowLayout(12, u->mStandardColorsPalette);
      lytStandard->setSpacing(2);
      //  lytStandard->setSizeConstraint(QLayout::SetFixedSize);
      KLFGridFlowLayout *lytCustom = new KLFGridFlowLayout(12, u->mCustomColorsPalette);
      lytCustom->setSpacing(2);
      //  lytCustom->setSizeConstraint(QLayout::SetFixedSize);
      */
  KLFFlowLayout *lytRecent = new KLFFlowLayout(u->mRecentColorsPalette, 11, 2, 2);
  lytRecent->setFlush(KLFFlowLayout::FlushBegin);
  KLFFlowLayout *lytStandard = new KLFFlowLayout(u->mStandardColorsPalette, 11, 2, 2);
  lytStandard->setFlush(KLFFlowLayout::FlushBegin);
  KLFFlowLayout *lytCustom = new KLFFlowLayout(u->mCustomColorsPalette, 11, 2, 2);
  lytCustom->setFlush(KLFFlowLayout::FlushBegin);

  connect(_recentcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteRecent()));
  connect(_standardcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteStandard()));
  connect(_customcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteCustom()));

  updatePalettes();

  int k;
  for (k = 0; k < _connectedColorChoosers.size(); ++k) {
    connect(_connectedColorChoosers[k], SIGNAL(colorChanged(const QColor&)),
	    this, SLOT(internalColorChanged(const QColor&)));
  }

  connect(u->lstNames, SIGNAL(itemClicked(QListWidgetItem*)),
	  this, SLOT(internalColorNameSelected(QListWidgetItem*)));
  connect(u->txtHex, SIGNAL(textChanged(const QString&)),
	  this, SLOT(internalColorNameSet(const QString&)));

  QPalette p = u->txtHex->palette();
  u->txtHex->setProperty("paletteDefault", QVariant::fromValue<QPalette>(p));
  p.setColor(QPalette::Base, QColor(255,169, 184,128));
  u->txtHex->setProperty("paletteInvalidInput", QVariant::fromValue<QPalette>(p));
  

  connect(u->btnAddCustomColor, SIGNAL(clicked()),
	  this, SLOT(setCurrentToCustomColor()));

  QStringList colornames = QColor::colorNames();
  for (k = 0; k < colornames.size(); ++k) {
    QPixmap colsample(16, 16);
    colsample.fill(QColor(colornames[k]));
    new QListWidgetItem(QIcon(colsample), colornames[k], u->lstNames);
  }

  internalColorChanged(_color);
}

void KLFColorChooseWidget::internalColorChanged(const QColor& wanted_newcolor)
{
  QColor newcolor = wanted_newcolor;
  if (!_alphaenabled)
    newcolor.setAlpha(255);

  int k;
  for (k = 0; k < _connectedColorChoosers.size(); ++k) {
    _connectedColorChoosers[k]->blockSignals(true);
    _connectedColorChoosers[k]->setProperty("color", QVariant(newcolor));
    _connectedColorChoosers[k]->blockSignals(false);
  }
  QString newcolorname = newcolor.name();
  if (u->txtHex->text() != newcolorname) {
    u->txtHex->blockSignals(true);
    u->txtHex->setText(newcolorname);
    u->txtHex->blockSignals(false);
  }

  _color = newcolor;

  emit colorChanged(newcolor);
}

void KLFColorChooseWidget::internalColorNameSelected(QListWidgetItem *item)
{
  if (!item)
    return;
  QColor color(item->text());
  internalColorChanged(color);
}

void KLFColorChooseWidget::internalColorNameSet(const QString& n)
{
  klfDbg("name set: "<<n) ;
  QString name = n;
  static QRegExp rx("\\#?[0-9A-Fa-f]{6}");
  bool validinput = false;
  bool setcolor = false;
  int listselect = -1;
  QColor color;
  if (rx.exactMatch(name)) {
    if (name[0] != QLatin1Char('#'))
      name = "#"+name;
    validinput = setcolor = true;
    color = QColor(name);
  } else {
    // try to match a color name, or the beginning of a color name
    int k;
    for (k = 0; k < u->lstNames->count(); ++k) {
      QString s = u->lstNames->item(k)->text();
      if (s == name) {
	// found an exact match. Select it and set color
	validinput = true;
	listselect = k;
	setcolor = true;
	color = QColor(name);
	break;
      }
      if (s.startsWith(n)) {
	// found a matching name. Just select it for user feedback
	validinput = true;
	listselect = k;
	setcolor = false;
	break;
      }
    }
  }
  // now set the background color of the text input correctly (valid input or not)
  if (!validinput) {
    u->txtHex->setProperty("invalidInput", true);
    u->txtHex->setStyleSheet(u->txtHex->styleSheet()); // style sheet recalc
    u->txtHex->setPalette(u->txtHex->property("paletteInvalidInput").value<QPalette>());
  } else {
    u->txtHex->setProperty("invalidInput", QVariant());
    u->txtHex->setStyleSheet(u->txtHex->styleSheet()); // style sheet recalc
    u->txtHex->setPalette(u->txtHex->property("paletteDefault").value<QPalette>());
  }
  // select the appropriate list item if needed
  if (listselect >= 0) {
    u->lstNames->blockSignals(true);
    u->lstNames->setCurrentRow(listselect, QItemSelectionModel::ClearAndSelect);
    u->lstNames->blockSignals(false);
  }
  if (setcolor)
    internalColorChanged(color);
}

void KLFColorChooseWidget::setColor(const QColor& color)
{
  if (color == _color)
    return;
  if (!_alphaenabled && color.rgb() == _color.rgb())
    return;

  internalColorChanged(color);
}

void KLFColorChooseWidget::setAlphaEnabled(bool enabled)
{
  _alphaenabled = enabled;
  u->spnAlpha->setShown(enabled);
  u->lblAlpha->setShown(enabled);
  u->mAlphaPane->setShown(enabled);
  u->lblsAlpha->setShown(enabled);
  u->mAlphaSlider->setShown(enabled);
  if (!enabled) {
    _color.setAlpha(255);
    setColor(_color);
  }
}

void KLFColorChooseWidget::fillPalette(KLFColorList *colorlist, QWidget *w)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("colorlist is "<<colorlist<<", _customcolors is "<<_customcolors<<", _recentcolors is "<<_recentcolors) ;
  int k;
  //  KLFGridFlowLayout *lyt = dynamic_cast<KLFGridFlowLayout*>( w->layout() );
  // KLF_ASSERT_NOT_NULL(lyt, "Layout is not a KLFGridFlowLayout !", return; ) ;
  KLFFlowLayout *lyt = dynamic_cast<KLFFlowLayout*>( w->layout() );
  KLF_ASSERT_NOT_NULL(lyt, "Layout is not a KLFFlowLayout !", return; ) ;

  lyt->clearAll();
  for (k = 0; k < colorlist->list.size(); ++k) {
    klfDbg("Adding a KLFColorClickSquare for color: "<<colorlist->list[k]) ;
    
    KLFColorClickSquare *sq = new KLFColorClickSquare(colorlist->list[k], 12,
						      (colorlist == _customcolors ||
						       colorlist == _recentcolors),
						      w);
    connect(sq, SIGNAL(colorActivated(const QColor&)),
	    this, SLOT(internalColorChanged(const QColor&)));
    connect(sq, SIGNAL(wantRemoveColor(const QColor&)),
    colorlist, SLOT(removeColor(const QColor&)));
    // lyt->insertGridFlowWidget(sq);
    lyt->addWidget(sq);
    sq->show();
  }
  w->adjustSize(); // the widget is inside a scroll area
}

void KLFColorChooseWidget::setCurrentToCustomColor()
{
  _customcolors->addColor(_color);
  updatePaletteCustom();
}

void KLFColorChooseWidget::updatePalettes()
{
  updatePaletteRecent();
  updatePaletteStandard();
  updatePaletteCustom();
}

void KLFColorChooseWidget::updatePaletteRecent()
{
  fillPalette(_recentcolors, u->mRecentColorsPalette);
}
void KLFColorChooseWidget::updatePaletteStandard()
{
  fillPalette(_standardcolors, u->mStandardColorsPalette);
}
void KLFColorChooseWidget::updatePaletteCustom()
{
  fillPalette(_customcolors, u->mCustomColorsPalette);
}



// static
void KLFColorChooseWidget::ensureColorListsInstance()
{
  if ( _recentcolors == 0 )
    _recentcolors = new KLFColorList(128);
  if ( _standardcolors == 0 )
    _standardcolors = new KLFColorList(256);
  if ( _customcolors == 0 )
    _customcolors = new KLFColorList(128);
}

// static
void KLFColorChooseWidget::addRecentColor(const QColor& col)
{
  ensureColorListsInstance();
  QList<QColor>::iterator it = _recentcolors->list.begin();
  while (it != _recentcolors->list.end()) {
    if ( (*it) == col )
      it = _recentcolors->list.erase(it);
    else
      ++it;
  }
  _recentcolors->list.append(col);

  if (_recentcolors->list.size() > MAX_RECENT_COLORS) {
    _recentcolors->list.removeAt(0);
  }
  _recentcolors->notifyListChanged();
}



// -------------------------------------------------------------------



void KLFColorList::addColor(const QColor& color)
{
  int i;
  if ( (i = list.indexOf(color)) >= 0 )
    list.removeAt(i);

  list.append(color);
  while (list.size() >= _maxsize)
    list.pop_front();

  emit listChanged();
}

void KLFColorList::removeColor(const QColor& color)
{
  bool changed = false;
  int i;
  if ( (i = list.indexOf(color)) >= 0 ) {
    list.removeAt(i);
    changed = true;
  }
  if (changed)
    emit listChanged();
}

// static
KLFColorList *KLFColorChooser::_colorlist = NULL;

QStyle *KLFColorChooser::mReplaceButtonStyle = NULL;

KLFColorChooser::KLFColorChooser(QWidget *parent)
  : QPushButton(parent), _color(0,0,0,255), _pix(), _allowdefaultstate(false),
    _defaultstatestring(tr("[ Default ]")), _autoadd(true), _size(120, 20),
    _xalignfactor(0.5f), _yalignfactor(0.5f), _alphaenabled(true), mMenu(NULL), menuRelFont(NULL)
{
  ensureColorListInstance();
  connect(_colorlist, SIGNAL(listChanged()), this, SLOT(_makemenu()));
  
  _makemenu();
  _setpix();

#ifdef Q_WS_MAC
  if ( mReplaceButtonStyle == NULL )
    mReplaceButtonStyle = new QPlastiqueStyle;
  setStyle(mReplaceButtonStyle);
#endif
}


KLFColorChooser::~KLFColorChooser()
{
}


QColor KLFColorChooser::color() const
{
  return _color;
}

QSize KLFColorChooser::sizeHint() const
{
  //KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width()+4;
  h = _pix.height()+2;

  opt.rect.setSize(QSize(w,h));

  if (menu())
    w += KLF_DEBUG_TEE( style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this) );

  //klfDbg("itermediate stage: w="<<w);

  QSize hint = style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this);
  //klfDbg("before expansion to app/globalstrut; hint="<<hint) ;
  hint = hint.expandedTo(QApplication::globalStrut());
  //klfDbg("mename="<<objectName()<<" _pix size="<<_pix.size()<<" _size="<<_size<<" color="<<_color<<"; sizeHint="<<hint) ;
  return hint;
}

void KLFColorChooser::setColor(const QColor& col)
{
  if ( ! _allowdefaultstate && ! col.isValid() )
    return;

  if (_color == col)
    return;

  _color = col;
  _setpix();

  if (_autoadd && _color.isValid()) {
    _colorlist->addColor(_color);
  }
  emit colorChanged(_color);
}

void KLFColorChooser::setDefaultColor()
{
  setColor(QColor());
}

void KLFColorChooser::setAllowDefaultState(bool allow)
{
  _allowdefaultstate = allow;
  _makemenu();
}
void KLFColorChooser::setDefaultStateString(const QString& str)
{
  _defaultstatestring = str;
  _makemenu();
}

void KLFColorChooser::setAutoAddToList(bool autoadd)
{
  _autoadd = autoadd;
}
void KLFColorChooser::setShowSize(const QSize& size)
{
  _size = size;
  _setpix();
  if (size.isValid())
    setMinimumSize(sizeHint());
  else
    setMinimumSize(QSize());
}
void KLFColorChooser::setPixXAlignFactor(float xalignfactor)
{
  _xalignfactor = xalignfactor;
}
void KLFColorChooser::setPixYAlignFactor(float yalignfactor) {
  _yalignfactor = yalignfactor;
}

void KLFColorChooser::setAlphaEnabled(bool on)
{
  _alphaenabled = on;
  _makemenu();
}

void KLFColorChooser::requestColor()
{
  // prefer our own color selection dialog
  QColor col = KLFColorDialog::getColor(_color, _alphaenabled, this);
  // QColor col = QColorDialog::getColor(_color, this);
  if ( ! col.isValid() )
    return;

  setColor(col);
}

void KLFColorChooser::setSenderPropertyColor()
{
  QColor c = sender()->property("setColor").value<QColor>();
  setColor(c);
}

void KLFColorChooser::_makemenu()
{
  if (mMenu) {
    setMenu(0);
    mMenu->deleteLater();
  }

  QSize menuIconSize = QSize(16,16);

  mMenu = new QMenu(this);

  if (_allowdefaultstate) {
    mMenu->addAction(QIcon(colorPixmap(QColor(), menuIconSize)), _defaultstatestring,
		     this, SLOT(setDefaultColor()));
    mMenu->addSeparator();
  }

  int n, k, nk;
  ensureColorListInstance();
  n = _colorlist->list.size();
  for (k = 0; k < n; ++k) {
    nk = n - k - 1;
    QColor col = _colorlist->list[nk];
    if (!_alphaenabled)
      col.setAlpha(255);
    QString collabel;
    if (col.alpha() == 255)
      collabel = QString("%1").arg(col.name());
    else
      collabel = QString("%1 (%2%)").arg(col.name()).arg((int)(100.0*col.alpha()/255.0+0.5));

    QAction *a = mMenu->addAction(QIcon(colorPixmap(col, menuIconSize)), collabel,
				  this, SLOT(setSenderPropertyColor()));
    a->setIconVisibleInMenu(true);
    a->setProperty("setColor", QVariant::fromValue<QColor>(col));
  }
  if (k > 0)
    mMenu->addSeparator();

  mMenu->addAction(tr("Custom ..."), this, SLOT(requestColor()));

  if (menuRelFont != NULL)
    delete menuRelFont;
  menuRelFont = new KLFRelativeFont(this, mMenu);
  menuRelFont->setRelPointSize(-1);
  setMenu(mMenu);
}

void KLFColorChooser::paintEvent(QPaintEvent *e)
{
  QPushButton::paintEvent(e);
  QPainter p(this);
  p.setClipRect(e->rect());
  p.drawPixmap(QPointF(_xalignfactor*(width()-_pix.width()), _yalignfactor*(height()-_pix.height())), _pix);
}

void KLFColorChooser::_setpix()
{
  //  if (_color.isValid()) {
  _pix = colorPixmap(_color, _size);
  // DON'T setIcon() because we draw ourselves ! see paintEvent() !
  //  setIconSize(_pix.size());
  //  setIcon(_pix);
  setText("");
  //  } else {
  //    _pix = QPixmap();
  //    setIcon(QIcon());
  //    setIconSize(QSize(0,0));
  //    setText("");
  //  }
}


QPixmap KLFColorChooser::colorPixmap(const QColor& color, const QSize& size)
{
  QPixmap pix = QPixmap(size);
  if (color.isValid()) {
    pix.fill(Qt::black);
    QPainter p(&pix);
    // background: a checker grid to distinguish transparency
    p.fillRect(0,0,pix.width(),pix.height(), QBrush(QPixmap(":/pics/checker.png")));
    // and fill with color
    p.fillRect(0,0,pix.width(),pix.height(), QBrush(color));
    //    pix.fill(color);
  } else {
    /*
     // draw "transparent"-representing pixmap
     pix.fill(QColor(127,127,127,80));
     QPainter p(&pix);
     p.setPen(QPen(QColor(255,0,0), 2));
     p.drawLine(0,0,size.width(),size.height());
    */
    // draw "default"/"transparent" pixmap
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient pgrad(0, 0, 0, 1);
    pgrad.setColorAt(0, QColor(160,160,185));
    pgrad.setColorAt(1, QColor(220,220,230));
    pgrad.setCoordinateMode(QGradient::StretchToDeviceMode);
    p.fillRect(0, 0, pix.width(), pix.height(), pgrad);

    QPen pen(QColor(127,0,0), 0.5f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.drawLine(QPointF(0,0), QPointF(pix.width(), pix.height()));
    p.drawLine(QPointF(0,pix.height()), QPointF(pix.width(), 0));

    /*
     //    p.scale((qreal)pix.width(), (qreal)pix.height());
     
     QRectF dashrect(QPointF(0.34*pix.width(), 0.40*pix.height()),
     QPointF(0.67*pix.width(), 0.60*pix.height()));
     //    QRectF dashrect(QPointF(0.1*pix.width(), 0.10*pix.height()),
     //    		    QPointF(0.9*pix.width(), 0.90*pix.height()));
     p.setClipRect(dashrect);
     p.translate(dashrect.topLeft());
     p.scale(dashrect.width(), dashrect.height());
     
     p.drawLine(0,0,1,1);
     
     QRadialGradient dashgrad(QPointF(0.75, 0.3), 0.4, QPointF(0.95, 0.2));
     dashgrad.setColorAt(0, QColor(180, 180, 240));
     dashgrad.setColorAt(1, QColor(40, 40, 50));
     dashgrad.setCoordinateMode(QGradient::LogicalMode);
     p.setPen(Qt::NoPen);
     p.setBrush(dashgrad);
     p.fillRect(QRectF(0,0,1,1), dashgrad);
    */

    //    qreal yrad = 2;
    //    qreal xrad = 2;//yrad * dashrect.height()/dashrect.width();
    //    p.drawRoundedRect(QRectF(0,0,1,1), xrad, yrad, Qt::AbsoluteSize);

    /*
    //    QLinearGradient pdashgrad(0, 0, 1, 0);
    //    pdashgrad.setColorAt(0, QColor(120, 0, 40));
    //    pdashgrad.setColorAt(1, QColor(120, 0, 40));
    QRadialGradient dashgrad(QPointF(1.75, 1.9), 0.6, QPointF(1.9, 1.8));
    //    QLinearGradient dashgrad(QPointF(0,0), QPointF(1,0));
    dashgrad.setColorAt(0, QColor(255, 0, 0));
    dashgrad.setColorAt(1, QColor(0, 255, 0));
    dashgrad.setCoordinateMode(QGradient::StretchToDeviceMode);
    //    dashgrad.setColorAt(0, QColor(255, 255, 255));
    //    dashgrad.setColorAt(1, QColor(40, 40, 50));
    //    QPen pen(QBrush(dashgrad), pix.height()/5.f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(Qt::NoPen);
    p.setBrush(dashgrad);
    QRectF dashrect(QPointF(0.34*pix.width(), 0.40*pix.height()),
		    QPointF(0.67*pix.width(), 0.65*pix.height()));
    qreal rad = pix.height()/8.;
    p.drawRoundedRect(dashrect, 1.2*rad, rad, Qt::AbsoluteSize);
    //    p.drawLine(pix.width()*3./8., pix.height()/2., pix.width()*5./8., pix.height()/2.);
    //    p.fillRect(0, 0, pix.width(), pix.height(), dashgrad); // debug this gradient
    */
  }
  return pix;
}



// static
int KLFColorChooser::staticUserMaxColors = 10;   // default of 10 colors


// static
void KLFColorChooser::setUserMaxColors(int maxColors)
{
  staticUserMaxColors = maxColors;
}

// static
void KLFColorChooser::ensureColorListInstance()
{
  if ( _colorlist == 0 )
    _colorlist = new KLFColorList(staticUserMaxColors);
}
// static
void KLFColorChooser::setColorList(const QList<QColor>& colors)
{
  ensureColorListInstance();
  _colorlist->list = colors;
  _colorlist->notifyListChanged();
}

// static
QList<QColor> KLFColorChooser::colorList()
{
  ensureColorListInstance();
  QList<QColor> l = _colorlist->list;
  return l;
}




