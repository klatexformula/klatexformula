/***************************************************************************
 *   file klfdebug.h
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

#ifndef KLFDEBUG_H
#define KLFDEBUG_H

#include <qmap.h>

#include <klfdefs.h>

// Note: function definitions are in klfdefs.cpp


// DEBUG UTILITIES (SOME WITH TIME PRINTING FOR TIMING OPERATIONS)


KLF_EXPORT QByteArray klfShortFuncSignature(const QByteArray& fullFuncName);
inline QByteArray klfShortFuncSignature(const char *fullFuncName)
#ifdef KLFBACKEND_QT4
{ return klfShortFuncSignature(QByteArray(fullFuncName)); }
#else
{ return klfShortFuncSignature(QByteArray().duplicate(fullFuncName, strlen(fullFuncName))); }
#endif

KLF_EXPORT QString klfTimeOfDay(bool shortFmt = true);

#ifdef KLFBACKEND_QT4
#  define KLF_SHORT_TIME qPrintable(klfTimeOfDay())
#else
#  define KLF_SHORT_TIME (klfTimeOfDay().ascii())
#endif


class KLF_EXPORT KLFDebugBlock
{
public:
  KLFDebugBlock(const QString& blockName);
  KLFDebugBlock(bool printmsg, const QString& blockName);

  virtual ~KLFDebugBlock();

protected:
  QString pBlockName;
private:
  bool pPrintMsg;
};

class KLF_EXPORT KLFDebugBlockTimer : public KLFDebugBlock
{
public:
  KLFDebugBlockTimer(const QString& blockName);
  virtual ~KLFDebugBlockTimer();
};

#ifndef KLFBACKEND_QT4
class KLF_EXPORT __klf_dbg_string_obj  {
  QString hdr;
public:
  __klf_dbg_string_obj(const QString& h) : hdr(h) { }
  __klf_dbg_string_obj(const __klf_dbg_string_obj& other) : hdr(other.hdr) { }
  int operator=(const QString& msg);
};
#endif

class KLFDebugObjectWatcherPrivate;

class KLF_EXPORT KLFDebugObjectWatcher : public QObject
{
  Q_OBJECT
public:
  static KLFDebugObjectWatcher *getWatcher();

  void registerObjectRefInfo(QObject *object, const QString& refInfo);
public slots:
  void debugObjectDestroyedFromSender() { debugObjectDestroyed(const_cast<QObject*>(sender())); }
  void debugObjectDestroyed(QObject *object);
private:

  KLFDebugObjectWatcher();
  virtual ~KLFDebugObjectWatcher();
  static KLFDebugObjectWatcher *instance;

  KLFDebugObjectWatcherPrivate *p;
};


#ifdef KLFBACKEND_QT4
// needed anyways for eg. KLF_ASSERT_CONDITION
#  include <QDebug>
#endif

#ifdef KLF_DEBUG

template<class T>
inline const T& __klf_debug_tee(const T& expr)
#  ifdef KLFBACKEND_QT4
{ qDebug()<<"TEE VALUE: "<<expr; return expr; }
#  else
{ return expr; } // sorry, no  qDebug()<<(anything you want)  in Qt 3 ...
#  endif


#  ifdef KLFBACKEND_QT4
KLF_EXPORT  QDebug
/* */ __klf_dbg_hdr(QDebug dbg, const char * funcname, const char *refinstance, const char * shorttime);
#  else
KLF_EXPORT  __klf_dbg_string_obj
/* */ __klf_dbg_hdr_qt3(const char *funcname, const char *refinstance, const char *shorttime) ;
#  endif

inline QString __klf_debug_ref_instance() { return QString(); }
#  define KLF_DEBUG_DECLARE_REF_INSTANCE( expr )			\
  protected: inline QString __klf_debug_ref_instance() const { return QString("[")+ (expr) + "]" ; }

#  define KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE()			\
  public: QString __klf_debug_this_ref_instance;			\
  protected: inline QString __klf_debug_ref_instance() const { return __klf_debug_this_ref_instance; }
#  define KLF_DEBUG_ASSIGN_REF_INSTANCE(object, ref_instance)	\
  (object)->__klf_debug_this_ref_instance = QString("[%1]").arg((ref_instance))
#  define KLF_DEBUG_ASSIGN_SAME_REF_INSTANCE(object)			\
  (object)->__klf_debug_this_ref_instance = __klf_debug_ref_instance();

#  define KLF_DEBUG_TIME_BLOCK(msg) KLFDebugBlockTimer __klf_debug_timer_block(QString("")+msg)
#  define KLF_DEBUG_BLOCK(msg) KLFDebugBlock __klf_debug_block(QString("")+msg)
#  define KLF_DEBUG_TEE(expr) __klf_debug_tee(expr)
#  define klfDebugf( arglist ) qDebug arglist
#  ifdef KLFBACKEND_QT4
#    define klfDbg( streamableItems )				\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, qPrintable(__klf_debug_ref_instance()), NULL) << streamableItems
#    define klfDbgT( streamableItems )					\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, qPrintable(__klf_debug_ref_instance()), KLF_SHORT_TIME) << streamableItems
#    define klfDbgSt( streamableItems )				\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, NULL, NULL) << streamableItems
#    define klfDbgStT( streamableItems )					\
  __klf_dbg_hdr(qDebug(), KLF_FUNC_NAME, NULL, KLF_SHORT_TIME) << streamableItems
#  else
#    define klfDbg( string )						\
  __klf_dbg_hdr_qt3(KLF_FUNC_NAME, __klf_debug_ref_instance().local8Bit(), NULL) = QString("") + (string)
#    define klfDbgT( string )						\
  __klf_dbg_hdr_qt3(KLF_FUNC_NAME, __klf_debug_ref_instance().local8Bit(), KLF_SHORT_TIME) = QString("") + (string)
#    define klfDbgSt( string )						\
  __klf_dbg_hdr_qt3(KLF_FUNC_NAME, NULL, NULL) = QString("") + (string)
#    define klfDbgStT( string )						\
  __klf_dbg_hdr_qt3(KLF_FUNC_NAME, NULL, KLF_SHORT_TIME) = QString("") + (string)
#  endif

#  define KLF_DEBUG_WATCH_OBJECT( qobj )				\
  { KLFDebugObjectWatcher::getWatcher()->registerObjectRefInfo((qobj), #qobj) ; \
    connect((qobj), SIGNAL(destroyed()),				\
	    KLFDebugObjectWatcher::getWatcher(), SLOT(debugObjectDestroyedFromSender())); \
  }


#else // KLF_DEBUG



#  define KLF_DEBUG_DECLARE_REF_INSTANCE( expr )
#  define KLF_DEBUG_DECLARE_ASSIGNABLE_REF_INSTANCE()
#  define KLF_DEBUG_ASSIGN_REF_INSTANCE(object, ref_instance)
#  define KLF_DEBUG_ASSIGN_SAME_REF_INSTANCE(object)


#  define KLF_DEBUG_TIME_BLOCK(msg)
#  define KLF_DEBUG_BLOCK(msg)

#  define KLF_DEBUG_TEE(expr) (expr)

#  define klfDebugf(arglist)

#  define klfDbg( streamableItems )
#  define klfDbgT( streamableItems )
#  define klfDbgSt( streamableItems )
#  define klfDbgStT( streamableItems )

#  define KLF_DEBUG_WATCH_OBJECT( qobj )

#endif // KLF_DEBUG



/* Ensure a usable __func__ symbol */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
#  if defined(__GNUC__) && __GNUC__ >= 2
#    define __func__ __FUNCTION__
#  else
#    ifdef KLFBACKEND_QT4
#      define __func__ (qPrintable(QString("<in %2 line %1>").arg(__LINE__).arg(__FILE__)))
#    else
#      define __func__ (QString("<in %2 line %1>").arg(__LINE__).arg(__FILE__).ascii().data())
#    endif
#  endif
#endif
/* The following declaration tests are inspired from "qglobal.h" in Qt 4.6.2 source code */
#ifndef KLF_FUNC_NAME
#  if (defined(Q_CC_GNU) && !defined(Q_OS_SOLARIS)) || defined(Q_CC_HPACC) || defined(Q_CC_DIAB)
#    define KLF_FUNC_NAME  (klfShortFuncSignature(__PRETTY_FUNCTION__).data())
#  elif defined(_MSC_VER)
    /* MSVC 2002 doesn't have __FUNCSIG__ */
#    if _MSC_VER <= 1300
#        define KLF_FUNC_NAME  __func__
#    else
#        define KLF_FUNC_NAME  (klfShortFuncSignature(__FUNCSIG__).data())
#    endif
#  else
#    define KLF_FUNC_NAME __func__
#  endif
#endif




#ifdef KLFBACKEND_QT4
#  define KLF_ASSERT_CONDITION(expr, msg, failaction)		       \
  if ( !(expr) ) {						       \
    qWarning().nospace()<<"In function "<<KLF_FUNC_NAME<<":\n\t"<<msg; \
    failaction;							       \
  }
#else
#  define KLF_ASSERT_CONDITION(expr, msg, failaction)			\
  if ( !(expr) ) {							\
    qWarning("In function %s:\n\t%s", KLF_FUNC_NAME, (QString("")+msg).local8Bit().data()); \
    failaction;								\
  }
#endif
#define KLF_ASSERT_CONDITION_ELSE(expr, msg, failaction)	\
  KLF_ASSERT_CONDITION(expr, msg, failaction)			\
  else
#define KLF_ASSERT_NOT_NULL(ptr, msg, failaction)	\
  KLF_ASSERT_CONDITION((ptr) != NULL, msg, failaction)




#if defined(KLFBACKEND_QT4) && defined(QT_NO_DEBUG_OUTPUT)
// Qt fix: this line is needed in non-debug output mode (?)
inline QDebug& operator<<(QDebug& str, const QVariant& v) { return str; }
#endif





#endif
