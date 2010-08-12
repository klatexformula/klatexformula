/***************************************************************************
 *   file klfpathchooser.h
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


#ifndef KLFPATHCHOOSER_H
#define KLFPATHCHOOSER_H

#include <QFrame>
#include <QPushButton>
#include <QLineEdit>

#include <klfdefs.h>

/** \brief A widget comprising of a line edit and a "browse" button
 *
 * This widget can be used to open existing files, save files, and open existing directories.
 */
class KLF_EXPORT KLFPathChooser : public QFrame
{
  Q_OBJECT

  // mode: 0=open, 1=save, 2=choose dir
  Q_PROPERTY(int mode READ mode WRITE setMode)
  Q_PROPERTY(bool dialogConfirmOverwrite READ dialogConfirmOverwrite WRITE setDialogConfirmOverwrite)
  Q_PROPERTY(QString caption READ caption WRITE setCaption)
  Q_PROPERTY(QString filter READ filter WRITE setFilter)
  Q_PROPERTY(QString path READ path WRITE setPath USER true)
  
  Q_PROPERTY(bool possibleOverwriteWasConfirmed READ possibleOverwriteWasConfirmed)

public:
  KLFPathChooser(QWidget *parent);
  virtual ~KLFPathChooser();

  /** The path chooser's mode.
   * - \c 0 : choose an existing file for opening
   * - \c 1 : choose a (most likely non-existant) file for saving
   * - \c 2 : choose an existing directory
   */
  virtual int mode() const { return _mode; }
  virtual QString caption() const { return _caption; }
  virtual QString filter() const { return _filter; }
  virtual QString path() const;

  /** Returns the current \c dialogConfirmOverwrite setting. See setDialogConfirmOverwrite() */
  virtual bool dialogConfirmOverwrite() const { return _dlgconfirmoverwrite; }
  
  /** Whether the user has already been asked confirmation for overwrite or not
   *
   * This function returns TRUE if the path was obtained by the file dialog, and the
   * option dialogConfirmOverwrite is enabled. In other words, if the file were to
   * exist, then the dialog would have asked confirmation already.
   *
   * If this function returns FALSE, and the file exists, and the dialogConfirmOverwrite
   * setting is enabled, that means that the user entered the path manually and was NOT
   * prompted for overwrite confirmation.
   *
   * This value is only relevant in "save" mode. (see \ref mode())
   *
   * \bug WARNING: THIS FEATURE UNTESTED
   */
  virtual bool possibleOverwriteWasConfirmed() const { return _pathFromDialog; }

public slots:
  /** Set the path chooser's mode. See \ref mode() */
  virtual void setMode(int mode);
  virtual void setCaption(const QString& caption);
  virtual void setFilter(const QString& filter);

  /** Displays the path \c path in the widget.
   *
   * Note that if mode is save mode (mode()==1), the new path is flagged as not
   * overwrite-confirmed, ie. possibleOverwriteWasConfirmed() will return FALSE.
   */
  virtual void setPath(const QString& path);

  /** If this setting is set to TRUE, then the file dialog will ask to confirm before
   * selecting an existing file.
   *
   * Note that if the user entered manually the path, no overwrite confirmation will have
   * been required. see possibleOverwriteWasConfirmed().
   *
   * This option is only relevant in "save" mode (mode()==1). It has no effect in the other
   * modes.
   */
  virtual void setDialogConfirmOverwrite(bool confirm) { _dlgconfirmoverwrite = confirm; }

  virtual void requestBrowse();

private slots:
  void slotTextChanged();
  
private:
  int _mode;
  QString _caption;
  QString _filter;
  bool _dlgconfirmoverwrite;
  
  bool _pathFromDialog;

  QLineEdit *txtPath;
  QPushButton *btnBrowse;

  QString _selectedfilter;
};

#endif
