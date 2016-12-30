/***************************************************************************
 *   file klfdefs.h
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

#ifndef KLFDEFS_H_
#define KLFDEFS_H_

#include <QObject>
#include <QString>
#include <QVariant>


// EXPORTING SYMBOLS TO E.G. PLUGINS ...
#ifndef KLF_EXPORT
#  if defined(Q_OS_WIN)
#    if defined(KLF_SRC_BUILD)
#      define KLF_EXPORT __declspec(dllexport)
#    else
#      define KLF_EXPORT __declspec(dllimport)
#    endif
#  else
#    define KLF_EXPORT __attribute__((visibility("default")))
#  endif
#endif


// VERSION INFORMATION

KLF_EXPORT const char * klfVersion();

KLF_EXPORT int klfVersionMaj();
KLF_EXPORT int klfVersionMin();
KLF_EXPORT int klfVersionRelease();


KLF_EXPORT QByteArray klfFmt(const char * fmt, ...)
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

#define klfFmtCC   (const char*)klfFmt

KLF_EXPORT QByteArray klfFmt(const char * fmt, va_list pp) ;

KLF_EXPORT QByteArray klfFmtDouble(double num, char fmt = 'g', int precision = 6);

#define klfFmtDoubleCC (const char*)klfFmtDouble


#define KLF_FUNC_SINGLE_RUN						\
  { static bool first_run = true;  if ( ! first_run )  return; first_run = false; }


#define KLF_DECLARE_PRIVATE(ClassName)					\
  private:								\
  ClassName##Private *d;						\
  friend struct ClassName##Private;					\
  inline ClassName##Private * d_func() { return d; }			\
  inline const ClassName##Private * d_func() const { return d; }

#define KLF_PRIVATE_HEAD(ClassName)				\
  private: ClassName *K;					\
  public:  ClassName * parentClass() const { return K; }        \
  public:  ClassName##Private (ClassName * ptr) : K(ptr)
#define KLF_PRIVATE_INHERIT_HEAD(ClassName, BaseInit)                   \
  private: ClassName *K;						\
  public:  ClassName * parentClass() const { return K; }                  \
  public:  ClassName##Private (ClassName * ptr) BaseInit, K(ptr)
#define KLF_PRIVATE_QOBJ_HEAD(ClassName, QObj)				\
  private: ClassName *K;						\
  public:  ClassName * parentClass() const { return K; }                \
  public:  ClassName##Private (ClassName * ptr) : QObj(ptr), K(ptr)

#define KLF_INIT_PRIVATE(ClassName)		\
  do { d = new ClassName##Private(this); } while (0)
#define KLF_DELETE_PRIVATE			\
  do { if (d != NULL) { delete d; } } while (0)


#define KLF_BLOCK							\
  for (bool _klf_block_first = true; _klf_block_first; _klf_block_first = false)

#define KLF_TRY(expr, msg, failaction)				       \
  if ( !(expr) ) {						       \
    klfWarning(msg);						       \
    failaction;							       \
  }





#define KLF_DEFINE_PROPERTY_GET(ClassName, type, prop)	\
  type ClassName::prop() const { return d_func()->prop; }

#define KLF_DEFINE_PROPERTY_GETSET(ClassName, type, prop, Prop)	\
  KLF_DEFINE_PROPERTY_GET(ClassName, type, prop)		\
    void ClassName::set##Prop(type value) { d_func()->prop = value; }

#define KLF_DEFINE_PROPERTY_GETSET_C(ClassName, type, prop, Prop)	\
  KLF_DEFINE_PROPERTY_GET(ClassName, type, prop)			\
    void ClassName::set##Prop(const type& value) { d_func()->prop = value; }




// utility functions

KLF_EXPORT bool klfIsValidVersion(const QString& v);

KLF_EXPORT int klfVersionCompare(const QString& v1, const QString& v2);

KLF_EXPORT bool klfVersionCompareLessThan(const QString& v1, const QString& v2);



// Import debugging utilities
#include <klfdebug.h>


#endif
