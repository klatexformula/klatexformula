/***************************************************************************
 *   file klfstdimplementation.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2019 by Philippe Faist
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

#ifndef KLFSTDIMPLEMENTATION_H
#define KLFSTDIMPLEMENTATION_H


#include <klfdefs.h>
#include <klfimplementation.h>


class KLFBackendDefaultCompilationTask;

struct KLFBackendDefaultImplementationPrivate;

class KLFBackendDefaultImplementation : public KLFBackendImplementation
{
  Q_OBJECT
public:
  KLFBackendDefaultImplementation(QObject * parent);
  virtual ~KLFBackendDefaultImplementation();

  KLFResultErrorStatus<KLFBackendCompilationTask *>
  createCompilationTask(const KLFBackendInput & input, const QVariantMap & parameters);

  static QStringList knownLatexEngines();

private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultImplementation) ;
};


struct KLBackendDefaultCompilationTaskPrivate;

class KLFBackendDefaultCompilationTask : public KLFBackendCompilationTask
{
  Q_OBJECT
public:
  KLFBackendDefaultCompilationTask(const KLFBackendInput & input, const KLFBackendSettings & settings,
                                   KLFBackendDefaultImplementation * implementation);
  virtual ~KLFBackendDefaultCompilationTask();

  virtual QStringList availableFormats() const;

protected:
  virtual KLFResultErrorStatus<QByteArray>
  compileToFormat(const QString & format, const QVariantMap & parameters);

  virtual QVariantMap canonicalParameters(const QString & format, const QVariantMap & parameters);

private:
  KLF_DECLARE_PRIVATE(KLFBackendDefaultCompilationTask) ;
}





#endif
