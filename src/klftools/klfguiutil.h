/***************************************************************************
 *   file klfguiutil.h
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

#ifndef KLFGUIUTIL_H
#define KLFGUIUTIL_H


#include <QString>
#include <QByteArray>
#include <QComboBox>
#include <QWidget>
#include <QDialog>
#include <QProgressDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QTime>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMovie>
#include <QPainter>

#include <klfutil.h>


//! Object that emits progress information of a (lengthy) operation
/**
 * This object is intented to be used in two ways
 * - to get informed about progress of an operation (for waiters)
 * - to report progress information of an operation (for progress reporters)
 *
 * To get informed, simple connect to the \ref progress(int) signal, and possibly
 * the \ref finished() signal, to respectively get informed about how much progress
 * is going on, and when the operation is finished. See the doc of those respective
 * functions for more info.
 *
 * To inform others about progress of an operation you're preforming, create a dedicated
 * instance of KLFProgressReporter for that specific operation, inform others of the
 * existance of such an object (to let them connect to progress()), then call
 * doReportProgress() at regular intervals, making sure to call it the last time with the
 * max() value.
 */
class KLF_EXPORT KLFProgressReporter : public QObject
{
  Q_OBJECT
public:
  KLFProgressReporter(int min, int max, QObject *parent = NULL);
  /** \note emits finished() if hasn't done so already. */
  virtual ~KLFProgressReporter();

  inline int min() const { return pMin; }
  inline int max() const { return pMax; }

signals:
  /** Emitted at regular intervals to inform connected slots about the progress of
   * a given action.
   *
   * This signal is emitted repeatedly with increasing \c progressValue values ranging from
   * \ref min() to \ref max().
   *
   * The last time this signal is emitted for one operation, its \c progressValue is exactly
   * the \ref max() value. (Progress reporters must enforce this).
   */
  void progress(int progressValue);

  /** Emitted right after progress() was emitted with the max() value.
   * If doReportProgress() is never called with the max() value, then this signal is
   * emitted in the destructor.
   */
  void finished();

public slots:
  /** The operations that perform long operations should regularly call this function.
   * This function emits progress() with the given value.
   *
   * Additionally, if value is max(), finished() is emitted. Do NOT call doReportProgress()
   * any more after that, it will result in a warning.
   */
  void doReportProgress(int value);

private:
  int pMin;
  int pMax;
  bool pFinished;
};

/** \brief A Progress Dialog
 *
 * This class is a QProgressDialog derivative that is optimized to work for klatexformula's needs,
 * especially making it easy to use with KLFProgressReporter.
 *
 * Among others, this dialog provides a straightforward option to disable the Cancel button.
 *
 * Basically this is just a wrapper around QProgressDialog's functions. Typical use is:
 * \code
 *  MyWidget::someLengthyOperation()
 *  {
 *    int number_of_steps = ...;
 *    KLFProgressDialog pdlg(false, this); // disable cancel
 *    KLFProgressReporter progressReporter(0, number_of_steps);
 *    pdlg.startReportingProgress(&progressReporter, "Please wait until some long operation completes...");
 *    ...
 *    int step = 0;
 *    for (step = 0; step < number_of_steps; ++step) {
 *      // an iteration step
 *      ...
 *      progressReporter.doReportProgress(step)
 *    }
 *  }
 * \endcode
 *
 * The previous example is somewhat trivial and does not exhibit the advantages of the features of
 * this class and of KLFProgressReporter over QProgressDialog; the example of KLFLibResourceEngine
 * is more relevant:
 * \code
 * // klflib.h  : the library resource engine system (no GUI)
 * class KLFLibResourceEngine {
 *    ....
 * signals:
 *   void operationStartReportingProgress(KLFProgressReporter *progressReporter, const QString& label);
 *
 *   ...
 * };
 *
 * // klflibbrowser.cpp  : the library browser (GUI)
 *   ...
 * bool KLFLibBrowser::openResource(...)
 * {
 *   ...
 *   connect(resource, SIGNAL(operationStartReportingProgress(KLFProgressReporter *, const QString&)),
 *	  this, SLOT(slotStartProgress(KLFProgressReporter *, const QString&)));
 *   ...
 * }
 * void KLFLibBrowser::slotStartProgress(KLFProgressReporter *progressReporter, const QString& label)
 * {
 *   KLFProgressDialog *pdialog = new KLFProgressDialog(false, QString(), this);
 *   pdialog->startReportingProgress(progressReporter, label);
 *   ...
 * }
 * \endcode
 * which opens a progress dialog whenever the resource emits an <tt>operationStartReportingProgress()</tt>
 * signal. Note that in this example, we have not provided the means to delete the progress dialog once
 * it has completed; for details have a look at the source code of klflibbrowser.cpp.
 *
 * For yet another example of inline (on-the-stack) usage of KLFProgressDialog, check out
 * KLFLibBrowser::slotExport() in klflibbrowser.cpp.
 *
 * \todo .... I'm not sure, but to delete the dialog, couldn't we simply connect
 *   KLFProgressReporter::finished() to KLFProgressDialog::deleteLater() ... ? needs a test.
 *
 */
class KLF_EXPORT KLFProgressDialog : public QProgressDialog
{
  Q_OBJECT
public:
  /** Build a progress dialog with the given labelText and parent */
  KLFProgressDialog(QString labelText = QString(), QWidget *parent = NULL);

  /** Build a progress dialog with a cancel button that is enabled or disabled (following the
   * value of \c canCancel), with label \c labelText, and parent \c parent. */
  KLFProgressDialog(bool canCancel, QString labelText, QWidget *parent);

  virtual ~KLFProgressDialog();


public slots:

  /** same as QProgressDialog::setLabelText() but resizes the dialog a bit larger so that its looks
   * nicer. */
  virtual void setDescriptiveText(const QString& labelText);
  /** start reporting progress from \c progressReporter and set label text to \c descriptiveText.
   *
   * \note This will disconnect the previously set progress reporter to the slot \c setValue() */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter,
				      const QString& descriptiveText);
  /** start reporting progress from \c progressReporter, without changing label text.
   *
   * \note This will disconnect the previously set progress reporter to the slot \c setValue() */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter);

  /** Calls directly QProgressDialog::setValue() */
  virtual void setValue(int value);

protected:
  void paintEvent(QPaintEvent *event);

private:
  void setup(bool canCancel);
  void init(const QString& labelText);

  KLFProgressReporter *pProgressReporter;

  bool pGotPaintEvent;
};


/** \brief A popup screen inviting the user to wait
 *
 * A splashscreen-like widget that appears in the middle of the screen displaying a text
 * inviting the user to wait.
 *
 * Used in klatexformula for example when changing skins, to invite the user to wait until
 * Qt finished processing the new style sheet rules onto all the widgets.
 *
 * The popup is closed if the user clicks it. However this works only if GUI events are
 * processed regularly by the caller (which is not the case if for example you use this
 * utility in a function taking much time to execute, and that blocks the GUI)
 *
 * You may choose to disable the parent widget while this popup is shown, for example to
 * forbid the user from editing any fields while a change is being processed. Enable this
 * option (by default false) with setDisableUi(). If disableUi is set, then the parent widget
 * is disabled when showPleaseWait() is called, and re-enabled again when this popup is
 * destroyed.
 *
 * See documentation for the constructor KLFPleaseWaitPopup() for an example.
 */
class KLF_EXPORT KLFPleaseWaitPopup : public QLabel
{
  Q_OBJECT
  Q_PROPERTY(bool disableUi READ willDisableUi WRITE setDisableUi) ;
public:
  /** Build a KLFPleaseWaitPopup object. This behaves very much like a regular Qt window widget,
   * like QDialog or splsh screen.
   *
   * The common use is to create it on the stack, eg.
   * \code
   *   void MyWidget::longFunction() {
   *     KLFPleaseWaitPopup popup(..., this);
   *     popup.showPleaseWait();
   *     ... // long execution time code, with if possible regular calls to qApp->processEvents()
   *   }
   * \endcode
   *
   * If \c alwaysAbove is TRUE, then the widget will set some window flags that will display it
   * above all other windows. Note that if this widget is displayed and no application events can
   * be delivered (because of a long, GUI-blocking operation), this can be obstrusive as the window
   * will display over all other windows and cannot be closed by clicking into it. If \c alwaysAbove
   * is FALSE, then the popup will still display above the \c callingWidget.
   */
  KLFPleaseWaitPopup(const QString& text, QWidget *callingWidget = NULL, bool alwaysAbove = false);
  virtual ~KLFPleaseWaitPopup();

  inline bool willDisableUi() const { return pDisableUi; }

  /** If set to TRUE, then calling showPleaseWait() will disable the widget passed to
   * the constructor. The widget is re-enabled when this popup is destroyed.
   *
   * Default is false. */
  virtual void setDisableUi(bool disableUi);

  /** Returns TRUE as soon as this widget got its first paint event. */
  virtual bool pleaseWaitShown() const { return pGotPaintEvent; }

  /** Returns TRUE if the user clicked on the popup to hide it. (The popup is automatically
   * hidden in that case.) */
  virtual bool wasUserDiscarded() const { return pDiscarded; }

public slots:
  /** Show the "please wait" widget. */
  virtual void showPleaseWait();

protected:
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);

private:
  QWidget *pParentWidget;
  bool pDisableUi;
  bool pGotPaintEvent;
  bool pDiscarded;
};


/** \brief A popup screen inviting user to wait, appearing after a delay
 *
 * See KLFPleaseWaitPopup.
 *
 * Additionally, this class provides setDelay(int) where you can specify after how much
 * time this popup should be displayed.
 *
 * Your code should regularly call process(). process() will check current time,
 * and show the widget if the delay has elapsed from the time the widget was
 * constructed.
 * */
class KLF_EXPORT KLFDelayedPleaseWaitPopup : public KLFPleaseWaitPopup
{
  Q_OBJECT
public:
  KLFDelayedPleaseWaitPopup(const QString& text, QWidget *callingWidget = NULL);
  virtual ~KLFDelayedPleaseWaitPopup();

  virtual void setDelay(int ms);

public slots:
  virtual void process();

private:
  int pDelay;
  QTime timer;
};


/** \brief a combo box proposing a list of (integer) enumeration values.
 *
 * Utility class built over QComboBox that can be used to propose a list of enumeration values
 * to user.
 *
 * You can set the enumeration values in the constructor or with setEnumValues(), and retrieve
 * the currently selected enum value with selectedValue().
 *
 * \note We say "enumeration values" because that is this widget's main purpose, however of
 *   course this is just a widget that more conveniently connects a title displayed to the
 *   user with an integer, and allows the user to select the former while the program code
 *   manipulates the latter.
 *
 * \note Note also that this widget sets item data in the QComboBox with setItemData() using
 *   the default role number. If you want also to set item data, use another role number!
 */
class KLF_EXPORT KLFEnumComboBox : public QComboBox
{
  Q_OBJECT

  Q_PROPERTY(int selectedValue READ selectedValue WRITE setSelectedValue)
public:
  KLFEnumComboBox(QWidget *parent = 0);
  KLFEnumComboBox(const QList<int>& enumValues, const QStringList& enumTitles,
		  QWidget *parent = 0);
  virtual ~KLFEnumComboBox();

  int selectedValue() const;

  QString enumText(int enumValue) const;

signals:
  void selectedValueChanged(int enumValue);

public slots:
  void setSelectedValue(int val);

  void setEnumValues(const QList<int>& enumValues, const QStringList& enumTitles);

private slots:
  void internalCurrentIndexChanged(int index);

private:
  QList<int> pEnumValueList;
  QMap<int,QString> pEnumValues;
  QMap<int,int> pEnumCbxIndexes;
};





/** \brief A Layout that lays out its children in a grid, flowing left to right, top to bottom
 *
 * Used eg. in KLF's color dialog to display the standard color panels
 *
 * Be sure to insert items into the layout with \ref insertGridFlowWidget()
 */
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




//! An animation display
/** This animation widget can be used as an overlay widget (meaning, not positioned within a layout)
 * to indicate the user to be patient.
 *
 * Note that this label relies on a non-NULL parent widget. <i>(Exception: the parent is only needed in
 * calcAnimationLabelGeometry(); if you need a parentless animation widget, subclass this animation
 * label and reimplement that function to fit your needs without calling the base implemenation
 * of that function)</i>.
 */
class KLFWaitAnimationOverlay : public QLabel
{
  Q_OBJECT
  Q_PROPERTY(QString waitMovie READ waitMovieFileName WRITE setWaitMovie) ;
  Q_PROPERTY(int widthPercent READ widthPercent WRITE setWidthPercent) ;
  Q_PROPERTY(int heightPercent READ heightPercent WRITE setHeightPercent) ;
  Q_PROPERTY(int positionXPercent READ positionXPercent WRITE setPositionXPercent) ;
  Q_PROPERTY(int positionYPercent READ positionYPercent WRITE setPositionYPercent) ;
  Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor) ;
public:
  KLFWaitAnimationOverlay(QWidget *parent);
  virtual ~KLFWaitAnimationOverlay();

  inline QString waitMovieFileName() const { return (pAnimMovie!=NULL) ? pAnimMovie->fileName() : QString(); }

  inline int widthPercent() const { return pWidthPercent; }
  inline int heightPercent() const { return pHeightPercent; }
  inline int positionXPercent() const { return pPositionXPercent; }
  inline int positionYPercent() const { return pPositionYPercent; }

  QColor backgroundColor() const;

public slots:
  /** \brief Set which animation to display while searching
   *
   * An animation is displayed when performing long searches, to tell the user to be patient. A default
   * animation is provided if you do not call this function. If you give a NULL movie pointer, the animation
   * is unset and disabled.
   *
   * The ownership of \c movie is transferred to this search bar object, and will be \c delete'd when no
   * longer used.
   */
  virtual void setWaitMovie(QMovie *movie);
  /** Set the animation to display while searching (eg. MNG file). See also setWaitMovie(QMovie*).
   */
  virtual void setWaitMovie(const QString& file);

  //! Sets the width of this label
  /** Sets the width of the displayed animation, in percent of the parent's width.
   * \c 50% will occupy half of the parent's width, leaving \c 25% on each side, while
   * \c 100% will occupy the full parent width.
   *
   * This function has no effect if \ref calcAnimationLabelGeometry() has been reimplemented in a subclass
   * that does not call the base implementation of that function.
   *
   * This function must be called before animation is shown with startWait().
   *
   * See also setHeightPercent().
   */
  void setWidthPercent(int widthpercent) { pWidthPercent = widthpercent; }

  //! Sets the height of this label
  /** See setWidthPercent().
   */
  void setHeightPercent(int heightpercent) { pHeightPercent = heightpercent; }

  //! Sets the horizontal position of this label relative to the parent widget
  /** The value given is, in percent, the amout of space on the left of this label (relative to parent), with
   * \c 0% being aligned completely to the left (no space left on the left) and \c 100% being aligned completely
   * to the right (no space left on the right). \c 50% will center the label.
   *
   * The label will never go beyond the parent widget's geometry.
   *
   * This function has no effect if \ref calcAnimationLabelGeometry() has been reimplemented in a subclass
   * that does not call the base implementation of that function.
   *
   * See also setPositionYPercent().
   */
  void setPositionXPercent(int xpc) { pPositionXPercent = xpc; }

  //! Sets the vertical position of this label relative to the parent widget
  /** See setPositionXPercent().
   */
  void setPositionYPercent(int ypc) { pPositionYPercent = ypc; }

  //! Set the label background color.
  /** This function will set the label background color. It may contain an alpha value to make the label
   * translucent or semi-translucent.
   *
   * This function internally sets a style sheet to this label.
   */
  void setBackgroundColor(const QColor& c);

  /** \brief Display the animation */
  virtual void startWait();

  /** \brief Hide the animation */
  virtual void stopWait();

protected:
  virtual void timerEvent(QTimerEvent *event);

  /** Calculate the geometry the label should have, according to current parent geometry. This function is
   * called just before the label is shown.
   *
   * The returned QRect should be relative to the parent widget.
   */
  virtual QRect calcAnimationLabelGeometry();

private:
  bool pIsWaiting;
  QMovie *pAnimMovie;
  int pAnimTimerId;

  int pWidthPercent;
  int pHeightPercent;
  int pPositionXPercent;
  int pPositionYPercent;
};




/** \brief Draws the given image with a glow effect.
 *
 * Draws a glow effect for image \c foreground by extracting the alpha channel, and
 * creating fills an image of color \c glow_color with the same alpha channel. The
 * image is then drawn at all points (x,y) around (0,0) such that
 * <tt>|(x,y)-(0,0)| &lt; r</tt>, effectively overlapping the glow image with itself
 * creating a blur effect.
 *
 * The resulting graphics are painted using the painter \c painter, at the reference
 * position <tt>(0,0)</tt>. If you want your image drawn at another position, use
 * QPainter::translate().
 *
 * If \c also_draw_image is TRUE, then the image itself is also drawn on top of the
 * glow effect.
 */
KLF_EXPORT void klfDrawGlowedImage(QPainter *painter, const QImage& foreground,
				   const QColor& glow_color = QColor(128, 255, 128, 8),
				   int radius = 4, bool also_draw_image = true);





#endif
