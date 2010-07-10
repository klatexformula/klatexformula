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
class KLFProgressReporter : public QObject
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


class KLFProgressDialog : public QProgressDialog
{
  Q_OBJECT
public:
  KLFProgressDialog(QString labelText = QString(), QWidget *parent = NULL);
  KLFProgressDialog(bool canCancel, QString labelText = QString(), QWidget *parent = NULL);
  virtual ~KLFProgressDialog();


public slots:

  virtual void setDescriptiveText(const QString& labelText);
  /** start reporting progress from \c progressReporter and set label text to \c descriptiveText. */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter,
				      const QString& descriptiveText);
  /** start reporting progress from \c progressReporter, without changing label text. */
  virtual void startReportingProgress(KLFProgressReporter *progressReporter);

  virtual void setValue(int value);

protected:
  void paintEvent(QPaintEvent *event);

private:
  void setup(bool canCancel);
  void init(const QString& labelText);
};


class KLFPleaseWaitPopup : public QLabel
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

  virtual void setDisableUi(bool disableUi);

  virtual bool pleaseWaitShown() { return pGotPaintEvent; }

public slots:
  virtual void showPleaseWait();

protected:
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);

private:
  QWidget *pParentWidget;
  bool pDisableUi;
  bool pGotPaintEvent;
};


class KLFDelayedPleaseWaitPopup : public KLFPleaseWaitPopup
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


/** Utility class for a combo box proposing a list of enumeration values.
 */
class KLFEnumComboBox : public QComboBox
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
