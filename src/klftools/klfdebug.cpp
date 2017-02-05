/***************************************************************************
 *   file klfdebug.cpp
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


// All definitions are in klfdefs.cpp. I may choose to move them here in the future.
// ### Aug 2012: adding a template qt msg handler tool here.

// this file is here to fool automoc to create a klfdebug.moc.cpp for klfdebug.h
// in klfkateplugin.


#include <stdio.h>

#include <QDebug>
#include <QByteArray>
#include <QDateTime>
#include <QApplication>
#include <QMessageBox>

#include <klfdefs.h>
#include <klfdebug.h>



// define a template qt message handler:

// DEBUG, WARNING AND FATAL MESSAGES HANDLER

// redirect debug output to this file (if non-NULL) instead of stderr
static FILE *klf_qt_msg_fp = NULL;

// in case we want to print messages directly into terminal
static FILE *klf_fp_tty = NULL;
static bool klf_fp_tty_failed = false;


// a buffer where we store all warning messages
static QByteArray klf_qt_msg_warnings_buffer;


// internal
static void ensure_tty_fp()
{
#ifdef Q_OS_UNIX
  if (klf_fp_tty == NULL && !klf_fp_tty_failed) {
    if ( !(klf_fp_tty = fopen("/dev/tty", "w")) ) {
      klf_fp_tty_failed = true;
    }
  }
#else
  Q_UNUSED(klf_fp_tty_failed) ;
#endif
}


/** \brief get the TTY file pointer.
 *
 * If the TTY is not yet open, this function tries to open the TTY as a file pointer and returns the
 * corresponding pointer. Use klf_qt_msg_tty_fp_failed() to check for failure.
 *
 * This function returns NULL on non-UNIX systems.
 */
KLF_EXPORT  FILE * klf_qt_msg_get_tty_fp()
{
  ensure_tty_fp();
  return klf_fp_tty;
}

/** \brief returns true if an attempt to open the tty fp failed.
 *
 * If a previous attempt to open the TTY was made, this function returns true if that attempt failed. If
 * not, attempts to open the TTY and returns true if the attempt failed.
 */
KLF_EXPORT  bool klf_qt_msg_tty_fp_failed()
{
  ensure_tty_fp();
  return klf_fp_tty_failed;
}

/** \brief redirect all debug/warning messages to a given file pointer */
KLF_EXPORT  void klf_qt_msg_set_fp(FILE * fp)
{
  klf_qt_msg_fp = fp;

  // display small message to indicate the redirection
  FILE *fout = klf_qt_msg_fp;
  if (fout == NULL)
    fout = stderr;
  fprintf(fout, "\n\n"
	  "-------------------------------------------------\n"
	  "  KLATEXFORMULA DEBUG OUTPUT\n"
	  "-------------------------------------------------\n"
	  "Redirected on %s\n\n",
	  qPrintable(QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate)));
}

/** \brief returns the contents of the warnings buffer. */
KLF_EXPORT  QByteArray klf_qt_msg_get_warnings_buffer()
{
  return klf_qt_msg_warnings_buffer;
}

KLF_EXPORT  void klf_qt_msg_clear_warnings_buffer()
{
  klf_qt_msg_warnings_buffer = QByteArray();
}

/** \brief a template handler function for Qt debugging/warning/critical messages etc.
 *
 * This function is suitable for use with \ref qInstallMsgHandler() .
 */
KLF_EXPORT  void klf_qt_msg_handle(QtMsgType type, const QMessageLogContext &/*context*/, const QString &msgstr)
{
  FILE *fout = stderr;
  if (klf_qt_msg_fp != NULL) {
    fout = klf_qt_msg_fp;
  }
  ensure_tty_fp();

  QByteArray msgbytes(msgstr.toLatin1());
  const char * msg = msgbytes.constData();

  switch (type) {
  case QtDebugMsg:
    // only with debugging enabled
#ifdef KLF_DEBUG
    fprintf(fout, "D: %s\n", msg);
    fflush(fout);
#endif
    break;
  case QtWarningMsg:
    fprintf(fout, "Warning: %s\n", msg);
    fflush(fout);
    // add the warning also to the warnings buffer.
    klf_qt_msg_warnings_buffer += QByteArray("Warning: ") + msg + "\n";
#ifdef KLF_DEBUG
    // in debug mode, also print warning messages to TTY (because they get lost in the debug messages!)
    if (klf_fp_tty)
      fprintf(klf_fp_tty, "Warning: %s\n", msg);
#endif

#if defined KLF_WS_WIN && defined KLF_DEBUG
#  define   SAFECOUNTER_NUM   10
    // only show dialog after having created a QApplication
    if (qApp != NULL && qApp->inherits("QApplication")) {
      static int safecounter = SAFECOUNTER_NUM;
      if (!QString::fromLocal8Bit(msg).startsWith("MNG error")) { // ignore these "MNG" errors...
	if (safecounter-- >= 0) {
	  QMessageBox::warning(0, "Warning",
			       QString("KLatexFormula System Warning:\n%1")
			       .arg(QString::fromLocal8Bit(msg)));
	}
      }
      if (safecounter == -1) {
	QMessageBox::information(0, "Information",
				 QString("Shown %1 system warnings. Will stop displaying them.").arg(SAFECOUNTER_NUM));
	safecounter = -2;
      }
      if (safecounter < -2) safecounter = -2;
    }
#endif
    break;
  case QtCriticalMsg:
    fprintf(fout, "Error: %s\n", msg);
    fflush(fout);
    // add the message also to the warnings buffer.
    klf_qt_msg_warnings_buffer += QByteArray("Error: ") + msg + "\n";
    //
    // These messages can be seen in the "system messages" (Settings -> Advanced
    // -> System Messages); no need for error dialog
    //
    // #ifdef KLF_WS_WIN
    //     if (qApp != NULL && qApp->inherits("QApplication")) {
    //       QMessageBox::critical(0, QObject::tr("Error", "[[KLF's Qt Message Handler: dialog title]]"),
    // 			    QObject::tr("KLatexFormula System Error:\n%1",
    // 					"[[KLF's Qt Message Handler: dialog text]]")
    // 			    .arg(QString::fromLocal8Bit(msg)));
    //     }
    // #endif
    break;
  case QtFatalMsg:
    fprintf(fout, "Fatal: %s\n", msg);
    fflush(fout);
#ifdef KLF_WS_WIN
    if (qApp != NULL && qApp->inherits("QApplication")) {
      QMessageBox::critical(0, QObject::tr("FATAL ERROR",
					   "[[KLF's Qt Message Handler: dialog title]]"),
			    QObject::tr("KLatexFormula System FATAL ERROR:\n%1",
					"[[KLF's Qt Message Handler: dialog text]]")
			    .arg(QString::fromLocal8Bit(msg)));
    }
#endif
    ::exit(255);
  default:
    fprintf(fout, "?????: %s\n", msg);
    fflush(fout);
    break;
  }
}




