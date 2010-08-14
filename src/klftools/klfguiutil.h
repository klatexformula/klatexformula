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
 * this classand of KLFProgressReporter over QProgressDialog; the example of KLFLibResourceEngine
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
 *   KLFProgressDialog *pdialog = new KLFProgressDialog(false, this);
 *   pdialog->startReportingProgress(progressReporter, text);
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
   * value of \c canCancel), with label \c labelText, and parent \c parent.
    */
  KLFProgressDialog(bool canCancel, QString labelText = QString(), QWidget *parent = NULL);

  virtual ~KLFProgressDialog();


public slots:

  /** same as QProgressDialog::setLabelText() but resizes the dialog a bit larger so that its looks
   * nicer. */
  virtual void setDescriptiveText(const QString& labelText);
  /** start reporting progress from \c progressReporter and set label text to \c descriptiveText. */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter,
				      const QString& descriptiveText);
  /** start reporting progress from \c progressReporter, without changing label text. */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter);

  /** Calls directly QProgressDialog::setValue() */
  virtual void setValue(int value);

protected:
  void paintEvent(QPaintEvent *event);

private:
  void setup(bool canCancel);
  void init(const QString& labelText);

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
 * See the constructor KLFPleaseWaitPopup() for an example.
 *
 */
class KLF_EXPORT KLFPleaseWaitPopup : public QLabel
{
  Q_OBJECT
public:
  /** \note this popup is made parentless. it will not be automatically destroyed.
   *    The common use is to create it on the stack, eg.
   * \code
   *   void MyWidget::longFunction() {
   *     KLFPleaseWaitPopup popup(..., this);
   *     popup.showPleaseWait();
   *     ... // long execution time code, with if possible regular calls to qApp->processEvents()
   *   }
   * \endcode
   */
  KLFPleaseWaitPopup(const QString& text, QWidget *callingWidget = NULL);
  virtual ~KLFPleaseWaitPopup();

  /** If set to TRUE, then calling showPleaseWait() will disable the widget passed to
   * the constructor. The widget is re-enabled when this popup is destroyed.
   *
   * Default is false. */
  virtual void setDisableUi(bool disableUi);

  /** Returns TRUE as soon as this widget got its first paint event. */
  virtual bool pleaseWaitShown() { return pGotPaintEvent; }

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



#endif
