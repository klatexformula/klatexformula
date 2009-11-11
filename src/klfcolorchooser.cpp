/***************************************************************************
 *   file klfcolorchooser.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2008 by Philippe Faist
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
#include <QStylePainter>
#include <QColorDialog>
#include <QPaintEvent>
#include <QStyle>
#include <QPlastiqueStyle>
#include <QStyleOptionButton>
#include <QRegExp>

#include "klfconfig.h"
#include "klfcolorchooser.h"


#define cstr(x) (x).toLocal8Bit().constData()



// -------------------------------------------------------------------

QColor KLFColorDialog::getColor(QColor startwith, QWidget *parent)
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

// -------------------------------------------------------------------

KLFColorClickSquare::KLFColorClickSquare(QColor color, int size, bool removable, QWidget *parent)
  : QWidget(parent), _color(color), _size(size), _removable(removable)
{
  setFocusPolicy(Qt::StrongFocus);
  setFixedSize(_size, _size);
  setContextMenuPolicy(Qt::DefaultContextMenu);
}
void KLFColorClickSquare::paintEvent(QPaintEvent */*event*/)
{
  QStylePainter p(this);
  p.fillRect(0, 0, width(), height(), QBrush(_color));
  if (hasFocus()) {
    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.backgroundColor = QColor(0,0,0,0);
    p.drawPrimitive(QStyle::PE_FrameFocusRect, option);
  }
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
}

void KLFColorChooseWidgetPane::setColor(const QColor& newcolor)
{
  _color = newcolor;
  update();
  emit colorChanged(_color);
}
void KLFColorChooseWidgetPane::setPaneType(const QString& panetype)
{
  QStringList strlist = panetype.split("+");
  _colorcomponent = strlist[0].toLower();
  _colorcomponent_b = strlist[1].toLower();
}
void KLFColorChooseWidgetPane::paintEvent(QPaintEvent */*e*/)
{
  QPainter p(this);
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
      _img.setPixel(x, y, colorFromValues(_color, xfac*x, yfac*y).rgba());
    }
  }
  p.drawImage(0, 0, _img);
  // draw crosshairs
  QColor hairscol = qGray(_color.rgb()) > 80 ? Qt::black : Qt::white;
  if ( ! _colorcomponent.isEmpty() && _colorcomponent != "fix" ) {
    p.setPen(QPen(hairscol, 1.f, Qt::DotLine));
    x = valueA()/xfac;
    if (x < 0) x = 0; if (x >= width()) x = width()-1;
    p.drawLine(x, 0, x, height());
  }
  if ( ! _colorcomponent_b.isEmpty() && _colorcomponent_b != "fix" ) {
    p.setPen(QPen(hairscol, 1.f, Qt::DotLine));
    y = valueB()/yfac;
    if (y < 0) y = 0; if (y >= height()) y = height()-1;
    p.drawLine(0, y, width(), y);
  }
}
void KLFColorChooseWidgetPane::mousePressEvent(QMouseEvent *e)
{
  double xfac = (double)valueAMax() / (_img.width()-1);
  double yfac = (double)valueBMax() / (_img.height()-1);
  int x = e->pos().x();
  int y = e->pos().y();

  setColor(colorFromValues(_color, x*xfac, y*yfac));
}
void KLFColorChooseWidgetPane::mouseMoveEvent(QMouseEvent *e)
{
  double xfac = (double)valueAMax() / (_img.width()-1);
  double yfac = (double)valueBMax() / (_img.height()-1);
  int x = e->pos().x();
  int y = e->pos().y();
  if (x < 0) x = 0; if (x >= width()) x = width()-1;
  if (y < 0) y = 0; if (y >= height()) y = height()-1;

  setColor(colorFromValues(_color, x*xfac, y*yfac));
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
  /*  printf("colorFromValues(%s/alpha=%d, %d, %d): My components:(%s+%s);\n", cstr(col.name()),
      col.alpha(), a, b, cstr(_colorcomponent), cstr(_colorcomponent_b)); */
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
  //  printf("\tnew color is (%s/alpha=%d);\n", cstr(col.name()), col.alpha());
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
  //  printf("\tand color is finally %s/alpha=%d\n", cstr(col.name()), col.alpha());
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


KLFColorChooseWidget::KLFColorChooseWidget(QWidget *parent)
  : QWidget(parent)
{
  setupUi(this);

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

  _connectedColorChoosers.append(mDisplayColor);
  _connectedColorChoosers.append(mHueSatPane);
  _connectedColorChoosers.append(mValPane);
  _connectedColorChoosers.append(mAlphaPane);
  _connectedColorChoosers.append(mColorTriangle);
  _connectedColorChoosers.append(mHueSlider);
  _connectedColorChoosers.append(mSatSlider);
  _connectedColorChoosers.append(mValSlider);
  _connectedColorChoosers.append(mRedSlider);
  _connectedColorChoosers.append(mGreenSlider);
  _connectedColorChoosers.append(mBlueSlider);
  _connectedColorChoosers.append(mAlphaSlider);
  _connectedColorChoosers.append(spnHue);
  _connectedColorChoosers.append(spnSat);
  _connectedColorChoosers.append(spnVal);
  _connectedColorChoosers.append(spnRed);
  _connectedColorChoosers.append(spnGreen);
  _connectedColorChoosers.append(spnBlue);
  _connectedColorChoosers.append(spnAlpha);

  KLFGridFlowLayout *lytRecent = new KLFGridFlowLayout(12, mRecentColorsPalette);
  lytRecent->setSpacing(2);
  KLFGridFlowLayout *lytStandard = new KLFGridFlowLayout(12, mStandardColorsPalette);
  lytStandard->setSpacing(2);
  KLFGridFlowLayout *lytCustom = new KLFGridFlowLayout(12, mCustomColorsPalette);
  lytCustom->setSpacing(2);

  connect(_recentcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteRecent()));
  connect(_standardcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteStandard()));
  connect(_customcolors, SIGNAL(listChanged()), this, SLOT(updatePaletteCustom()));

  updatePalettes();

  int k;
  for (k = 0; k < _connectedColorChoosers.size(); ++k) {
    connect(_connectedColorChoosers[k], SIGNAL(colorChanged(const QColor&)),
	    this, SLOT(internalColorChanged(const QColor&)));
  }

  connect(lstNames, SIGNAL(itemClicked(QListWidgetItem*)),
	  this, SLOT(internalColorNameSelected(QListWidgetItem*)));
  connect(txtHex, SIGNAL(textChanged(const QString&)),
	  this, SLOT(internalColorNameSet(const QString&)));

  connect(btnAddCustomColor, SIGNAL(clicked()),
	  this, SLOT(setCurrentToCustomColor()));

  QStringList colornames = QColor::colorNames();
  for (k = 0; k < colornames.size(); ++k) {
    QPixmap colsample(16, 16);
    colsample.fill(QColor(colornames[k]));
    new QListWidgetItem(QIcon(colsample), colornames[k], lstNames);
  }

  internalColorChanged(_color);
}

void KLFColorChooseWidget::internalColorChanged(const QColor& newcolor)
{
  int k;
  for (k = 0; k < _connectedColorChoosers.size(); ++k) {
    _connectedColorChoosers[k]->blockSignals(true);
    _connectedColorChoosers[k]->setProperty("color", QVariant(newcolor));
    _connectedColorChoosers[k]->blockSignals(false);
  }
  QString newcolorname = newcolor.name();
  if (txtHex->text() != newcolorname) {
    txtHex->blockSignals(true);
    txtHex->setText(newcolorname);
    txtHex->blockSignals(false);
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
  QString name = n;
  static QRegExp rx("\\#?[0-9A-Za-z]{6}");
  if (!rx.exactMatch(name)) {
    txtHex->setStyleSheet("background-color: rgb(255,128,128)");
    return;
  }
  txtHex->setStyleSheet("");
  if (name[0] != QLatin1Char('#'))
    name = "#"+name;
  QColor color(name);
  internalColorChanged(color);
}

void KLFColorChooseWidget::setColor(const QColor& color)
{
  if (color == _color)
    return;

  internalColorChanged(color);
}

void KLFColorChooseWidget::fillPalette(KLFColorList *colorlist, QWidget *w)
{
  int k;
  KLFGridFlowLayout *lyt = dynamic_cast<KLFGridFlowLayout*>( w->layout() );
  lyt->clearAll();
  for (k = 0; k < colorlist->list.size(); ++k) {
    KLFColorClickSquare *sq = new KLFColorClickSquare(colorlist->list[k], 12,
						      (colorlist == _customcolors || colorlist == _recentcolors),
						      w);
    connect(sq, SIGNAL(colorActivated(const QColor&)),
	    this, SLOT(internalColorChanged(const QColor&)));
    connect(sq, SIGNAL(wantRemoveColor(const QColor&)),
	      colorlist, SLOT(removeColor(const QColor&)));
    lyt->insertGridFlowWidget(sq);
    sq->show();
  }
  w->adjustSize();
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
  fillPalette(_recentcolors, mRecentColorsPalette);
}
void KLFColorChooseWidget::updatePaletteStandard()
{
  fillPalette(_standardcolors, mStandardColorsPalette);
}
void KLFColorChooseWidget::updatePaletteCustom()
{
  fillPalette(_customcolors, mCustomColorsPalette);
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
  : QPushButton(parent), _color(0,0,0,255), _pix(), _allowdefaultstate(false), _autoadd(true), _size(120, 20),
    _xalignfactor(0.5f), _yalignfactor(0.5f),
    mMenu(0)
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
  // inspired by QPushButton::sizeHint() in qpushbutton.cpp

  ensurePolished();

  int w = 0, h = 0;
  QStyleOptionButton opt;
  initStyleOption(&opt);

  // calculate contents size...
  w = _pix.width()+4;
  h = _pix.height()+2;

  if (menu())
    w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

  return (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
	  expandedTo(QApplication::globalStrut()));
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

void KLFColorChooser::requestColor()
{
  // prefer our own color selection dialog
  QColor col = KLFColorDialog::getColor(_color, this);
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
    mMenu->addAction(QIcon(colorPixmap(QColor(), menuIconSize)), tr("[ Default ]"), this, SLOT(setDefaultColor()));
    mMenu->addSeparator();
  }

  int n, k, nk;
  ensureColorListInstance();
  n = _colorlist->list.size();
  for (k = 0; k < n; ++k) {
    nk = n - k - 1;
    QAction *a = mMenu->addAction(QIcon(colorPixmap(_colorlist->list[nk], menuIconSize)), _colorlist->list[nk].name(),
				  this, SLOT(setSenderPropertyColor()));
    a->setProperty("setColor", QVariant(_colorlist->list[nk]));
  }
  if (k > 0)
    mMenu->addSeparator();

  mMenu->addAction(tr("Custom ..."), this, SLOT(requestColor()));

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
    pix.fill(color);
  } else {
    // draw "transparent"-representing pixmap
    pix.fill(QColor(127,127,127,80));
    QPainter p(&pix);
    p.setPen(QPen(QColor(255,0,0), 2));
    p.drawLine(0,0,size.width(),size.height());
  }
  return pix;
}



// static
void KLFColorChooser::ensureColorListInstance()
{
  if ( _colorlist == 0 )
    _colorlist = new KLFColorList(klfconfig.UI.maxUserColors);
}
// static
void KLFColorChooser::setColorList(const QList<QColor>& colors)
{
  ensureColorListInstance();
  //  *_colorlist = colors;
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

