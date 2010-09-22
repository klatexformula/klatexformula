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
#include <QDialog>
#include <QSpinBox>
#include <QGridLayout>
#include <QPainter>

#include <klfdefs.h>


class KLFColorList;


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

/** \brief Base utility class that stores and calculates specific components of a color that is
 * being edited.
 *
 * Example: a  <tt>Red: [...]</tt> spin-box would have to calculate the red component of a
 * newly set color, while a  <tt>Saturation: (.............)</tt> slider would have to calculate
 * the same, but for the saturation value.
 *
 * This class is a base class that stores a color one (respectively two) components are being edited
 * in a widget subclass. This class also provides some utilities to calculate the component values
 * from a color and vice versa.
 *
 * An editor is assumed to handle at most two components of a color (labels \a A and \a B indicate
 * respectively the first edited component and the second edited component). If the editor can edit
 * only one component (eg. spin-box), then the second component is set to the special component
 * \c "fix" and ignored by this class.
 *
 * The subclasses have to set \ref _colorcomponent (component \a A) and \ref _colorcomponent_b (component
 * \a B) directly, for example in their constructor.
 *
 * Valid components are: \c "hue", \c "sat", \c "val", \c "red", \c "green", \c "blue", \c "alpha",
 * and \c "fix". \c "fix" should be used when the component is not used (eg. 2nd component of a 1-D
 * slider). Components are case sensitive (all lower case).
 *
 * This class assumes that the editor subclass will be displayed in conjunction with other similar
 * editors, also KLFColorComponentsEditorBase subclasses (eg. a list of spin boxes to edit each component
 * of a color). It is therefore important to synchronize the various editors, because the information
 * that is displayed is redundant: for example red, green, blue editors may be displayed simultaniously
 * next to hue, saturation, and value editors. When for example the red component is changed, then the
 * saturation, value and hue will have to update.
 *
 * In the setup above, when one of the editors has its value changed, that editor that changed should
 * calculate the new color using refreshColorFromInternalValues() with the new component value(s), and
 * notify the other editors (eg. with signal-slot connections) of the change (note that the return value
 * of refreshColorFromInternalValues() indicates whether the color changed). All the other editors should
 * then calculate their new component values (with valueAFromNewColor() and valueBFromNewColor() to get
 * the values for component \a A and \a B respectively) and display those values.
 *
 * Note that all editors (in the setup described above) store independently the current color in their
 * \ref _color member. Eventually the color may be obtained by querying any of the editors.
 *
 * Note that both components may be set to \c "fix", for example for a widget that just displays the current
 * color (this is no longer an editor!).
 */
class KLF_EXPORT KLFColorComponentsEditorBase
{
protected:
  /** Returns the value of component \a A in the color \c color */
  int valueAFromNewColor(const QColor& color) const;
  /** Returns the value of component \a B in the color \c color */
  int valueBFromNewColor(const QColor& color) const;
  /** Returns the current value of component \a A in the color being edited (\ref _color) */
  inline int valueA() const { return valueAFromNewColor(_color); }
  /** Returns the current value of component \a B in the color being edited (\ref _color) */
  inline int valueB() const { return valueBFromNewColor(_color); }
  /** Returns the maximal possible value that can be assigned to component \a A */
  inline int valueAMax() const { return valueMax(_colorcomponent); }
  /** Returns the maximal possible value that can be assigned to component \a B */
  inline int valueBMax() const { return valueMax(_colorcomponent_b); }

  /** Calculate the color that we get when in \c color_base, we set the component \a A
   * value to \c value_a and component \a B to \c value_b.
   *
   * Example, with component \a A being red and component \a B being blue:
   * \code
   * colorFromValues(QColor(120, 60, 44), 10, 33)   ==   QColor(10, 60, 33)
   * \endcode
   * This example is somewhat trivial in that the displayed components are independant. However
   * this function can also handle the case when the components are not independant, eg.
   * component \a A being saturation and \a B being red.
   *
   * Example with \a A being Hue (\c "hue") and with \a B unused (\c "fix"):
   * \code
   * colorFromValues(QColor(20, 90, 120), 110)   ==   QColor(37, 120, 20)
   * \endcode
   * */
  QColor colorFromValues(QColor color_base, int value_a, int value_b = -1);

  /** Calls \ref colorFromNewValues() with the given values and with the current \ref _color, and
   * sets \ref _color to the returned value.
   *
   * \returns TRUE if the color changed, FALSE otherwise.
   */
  bool refreshColorFromInternalValues(int value_a, int value_b = -1);

  /** Returns the value of the component \c component in the color \c color.
   */
  static int valueFromNewColor(const QColor& color, const QString& component);

  /** Returns the Maximum value the given \c component may be assigned. This is \c 359 for hue,
   * \c 255 for others except \c 'fix', for which it is \c -1.
   *
   * Note that for all (valid) componenets minimum value is zero.
   */
  static int valueMax(const QString& component);

  /** The stored color that we are displaying components of */
  QColor _color;

  /** The components that we are displaying. The strings are one of those given in the
   * class main documentation: \ref KLFColorComponentsEditorBase. */
  QString _colorcomponent, _colorcomponent_b;
};


// ------------------------------------------------------------------------------------

/** \brief A Spin box editing a component of a color
 *
 * the color component is given as a string, one of those listed in documentation for
 * KLFColorComponentsEditorBase.
 *
 * Use \ref setColorComponent() to set the component, then use \ref setColor() to set
 * a color, and connect to \ref colorChanged() for changes by the user to this component,
 * and retrieve the color with \ref color(). color() returns the color that was set with
 * setColor(), except that the compoenent being edited was adjusted to the currently edited
 * value.
 *
 * You can retrieve the value of the component being edited with the value() member (inherited
 * from QSpinBox).
 *
 * Example usage is to have multiple instances of this spin box editing different components,
 * with a chain (looped) of colorChanged() signals connected to the setColor() of the next spin box
 * in such way as to have all spin boxes being up to date. With this trick you can edit components
 * that are not independant (eg. saturation and red values; they need to update each other when
 * edited).
 *
 * See \ref KLFColorChooseWidget for an example.
 */
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

/** \brief A pane displaying a gradient of colors, controlling one or two (arbitrary) components
 * of a color.
 *
 * Most common would be eg. hue and saturation in the big pane in most common selection dialogs.
 *
 * The pane type is the two components that this pane is editing, concatenated with a \c "+" sign,
 * eg. a red-blue 2-D editor pane would be described by the pane type \c "Red+Blue". Note that
 * pane types are case-insensitive and are converted to lower case.
 *
 * This class can also display only one editing dimension and keep the other fixed, just give
 * \c "fix" to that fixed dimension.
 *
 * For an example, look at the dynamic properties set in klfcolorchoosewidget.ui in each color
 * pane widget.
 *
 * The zeros of the components are placed top left of the pane.
 *
 */
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
  virtual void paintEvent(QPaintEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);

private:
  QImage _img;
};


// ------------------------------------------------------------------------------------

namespace Ui { class KLFColorChooseWidget; }
class QListWidgetItem;

/** A widget that displays a full-featured selection of sliders, spin boxes and standard
 * color controls that prompts the user to select a color.
 *
 * Set the color with setColor(), and retrieve it with color(). When the user changes the color,
 * a colorChanged() signal is emitted.
 *
 * You can allow or forbid the user to select transparent colors with setAlphaEnabled().
 *
 * For the "recent colors" and "custom colors" feature to work, you will have to manually save
 * those color lists into your application settings:
 * - upon starting up your application, read those color lists from some settings (eg. QSettings) and
 *   set them with the static function setRecentCustomColors().
 * - upon shutting down your application, save the recentColors() and customColors() to your settings.
 *
 */
class KLF_EXPORT KLFColorChooseWidget : public QWidget
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
  static void setRecentCustomColors(QList<QColor> recentcolors, QList<QColor> customcolors);
  static void addRecentColor(const QColor& col);
  static QList<QColor> recentColors();
  static QList<QColor> customColors();

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
  Ui::KLFColorChooseWidget *u;

  QColor _color;
  bool _alphaenabled;

  QList<QObject*> _connectedColorChoosers;

  void fillPalette(KLFColorList *colorlist, QWidget *w);

  static KLFColorList *_recentcolors;
  static KLFColorList *_standardcolors;
  static KLFColorList *_customcolors;
};


// ------------------------------------------------------------------------------------


namespace Ui { class KLFColorDialog; }

/** \brief A dialog to let the user select a color
 *
 * Invocation of the dialog is done 
 *
 * Possibility to select either solid colors only or arbitrarily transparent colors (alpha): set
 * the correct parameters to the static getColor() function.
 *
 */
class KLF_EXPORT KLFColorDialog : public QDialog
{
  Q_OBJECT
public:
  /** Constructor. If you build the dialog this way, you will have to initialize the \ref colorChooseWidget()
   * manually. Consider using \ref getColor() instead. */
  KLFColorDialog(QWidget *parent = 0);
  virtual ~KLFColorDialog();

  /** Accessor to the KLFColorChooseWidget that is displayed in the dialog. */
  KLFColorChooseWidget *colorChooseWidget();

  /** static method to invoke a new instance of the dialog, display it to user with the given settings (starts
   * displaying the color \c startwith, and allows the user to select (semi-)transparent colors if \c alphaenabled
   * is set).
   *
   * Returns the color which was chosen, or a <tt>QColor()</tt> if user canceled.
   */
  static QColor getColor(QColor startwith = Qt::black, bool alphaenabled = true, QWidget *parent = 0);

private:
  Ui::KLFColorDialog *u;
};




// ------------------------------------------------------------------------------------

class QStyle;

class KLF_EXPORT KLFColorChooser : public QPushButton
{
  Q_OBJECT

  Q_PROPERTY(QSize showSize READ showSize WRITE setShowSize)
  Q_PROPERTY(bool allowDefaultState READ allowDefaultState WRITE setAllowDefaultState)
  Q_PROPERTY(QString defaultStateString READ defaultStateString WRITE setDefaultStateString)
  Q_PROPERTY(bool autoAddToList READ autoAddToList WRITE setAutoAddToList)
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)
  Q_PROPERTY(float pixXAlignFactor READ pixXAlignFactor WRITE setPixXAlignFactor)
  Q_PROPERTY(float pixYAlignFactor READ pixYAlignFactor WRITE setPixYAlignFactor)
  Q_PROPERTY(bool alphaEnabled READ alphaEnabled WRITE setAlphaEnabled)

public:
  KLFColorChooser(QWidget *parent);
  ~KLFColorChooser();

  QSize showSize() const { return _size; }
  //! Allow the "default color" state
  /** This is NOT a default color in the sense that it's a normal color that will be returned
   * by default; it is a special state that can mean for ex. "no color", "full transparency" or
   * "don't change"; it is represented by a red slash on a gray background. It is internally
   * represented by an invalid QColor. */
  bool allowDefaultState() const { return _allowdefaultstate; }
  QString defaultStateString() const { return _defaultstatestring; }
  bool autoAddToList() const { return _autoadd; }
  QColor color() const;
  float pixXAlignFactor() const { return _xalignfactor; }
  float pixYAlignFactor() const { return _yalignfactor; }
  //! TRUE if the user can also select opacity (alpha) with this widget
  bool alphaEnabled() const { return _alphaenabled; }

  virtual QSize sizeHint() const;

  /** This function must be called before any instance is created, and before
   * calling setColorList() and/or colorList(), otherwise it has no effect. */
  static void setUserMaxColors(int maxcolors);

  static void setColorList(const QList<QColor>& colorlist);
  static QList<QColor> colorList();

signals:
  void colorChanged(const QColor& newcolor);

public slots:
  /** Sets the current color to \c color. If the \ref allowDefaultState() property
   * is TRUE, then the "default color" can be set with \ref setDefaultColor() or
   * \code setColor(QColor()) \endcode */
  void setColor(const QColor& color);
  void setAllowDefaultState(bool allow);
  void setDefaultStateString(const QString& str);
  void setAutoAddToList(bool autoadd) { _autoadd = autoadd; }
  void setShowSize(const QSize& size) { _size = size; }
  void setPixXAlignFactor(float xalignfactor) { _xalignfactor = xalignfactor; }
  void setPixYAlignFactor(float yalignfactor) { _yalignfactor = yalignfactor; }
  void setAlphaEnabled(bool alpha_enabled);
  /** equivalent to \code setColor(QColor()) \endcode */
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
  QString _defaultstatestring;
  bool _autoadd;
  QSize _size;
  float _xalignfactor, _yalignfactor;

  bool _alphaenabled;

  QMenu *mMenu;

  void _setpix();

  QPixmap colorPixmap(const QColor& color, const QSize& size);

  static KLFColorList *_colorlist;
  static QStyle *mReplaceButtonStyle;

  static int staticUserMaxColors;

  static void ensureColorListInstance();

};


#endif
