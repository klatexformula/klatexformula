/***************************************************************************
 *   file klfcmdiface.cpp
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

#include <QUrl>
#include <QUrlQuery>
#include <QGenericArgument>

#include <klfdatautil.h>

#include "klfcmdiface.h"


struct KLFCmdIfacePrivate
{
  KLF_PRIVATE_HEAD(KLFCmdIface)
  {
  }

  QMap<QString, QObject*> objects;
};


KLFCmdIface::KLFCmdIface(QObject *parent)
  : QObject(parent)
{
  KLF_INIT_PRIVATE(KLFCmdIface) ;
}
KLFCmdIface::~KLFCmdIface()
{
  KLF_DELETE_PRIVATE ;
}

void KLFCmdIface::registerObject(QObject *obj, const QString& name)
{
  if (d->objects.contains(name)) {
    qWarning()<<KLF_FUNC_NAME<<": Name "<<name<<" has already been registered.";
    return;
  }

  KLF_ASSERT_NOT_NULL(obj, "Cannot register NULL object!", return; ) ;

  d->objects[name] = obj;
}

// static
QUrl KLFCmdIface::encodeCommand(const Command& command)
{
  QUrl url;
  url.setScheme("klfcommand");
  url.setHost(command.object);
  url.setPath("/"+command.slot+"/");
  QVariantList args = command.args;
  QUrlQuery urlq;
  urlq.addQueryItem("klfUrlVersion", "3.3.0alpha"); // klf compatibility version
  for (int k = 0; k < args.size(); ++k) {
    QString key = QLatin1String("arg")+QString::number(k);
    QString val = QString::fromLatin1("[") + args[k].typeName() + "]"
      + QString::fromLatin1(klfSaveVariantToText(args[k]));
    urlq.addQueryItem(key, val);
  }
  url.setQuery(urlq);
  return url;
}

// static
KLFCmdIface::Command KLFCmdIface::decodeCommand(const QUrl& url)
{
  Command c;
  if (url.scheme() != "klfcommand") {
    qWarning()<<KLF_FUNC_NAME<<": Command url has wrong scheme "<<url.scheme();
    return c;
  }
  c.object = url.host();
  QString p = url.path();
  if (p.startsWith("/"))
    p = p.mid(1);
  if (p.endsWith("/"))
    p = p.mid(0, p.length()-1);
  c.slot = p;
  QUrlQuery urlq(url);
  // klf compat version
  QString klfver = urlq.queryItemValue("klfUrlVersion");
  // now decode args
  c.args = QVariantList();
  int k = 0;
  while (urlq.hasQueryItem("arg"+QString::number(k))) {
    QString val = urlq.queryItemValue("arg"+QString::number(k));
    QByteArray data = val.toLatin1();
    if (data[0] != '[') {
      c.args << QString::fromLocal8Bit(data);
      ++k;
      continue;
    }
    int ti = data.indexOf(']');
    if (ti == -1) {
      klfDbg("Malformed encoded data: "<<data) ;
      c.args << QVariant();
      ++k;
      continue;
    }
    QByteArray tname = data.mid(1, ti-1);
    QByteArray vdata = data.mid(ti+1);
    c.args << klfLoadVariantFromText(vdata, tname.constData());
    ++k;
    continue;
  }
  // args are decoded, we can return our result
  return c;
}

void KLFCmdIface::executeCommand(const QUrl& url)
{
  Command c = decodeCommand(url);

  if (!d->objects.contains(c.object)) {
    qWarning()<<KLF_FUNC_NAME<<": Name "<<c.object<<" has not been registered.";
    return;
  }

  QObject *target = d->objects.value(c.object, NULL);

  KLF_ASSERT_NOT_NULL(target, "Target is NULL!", return; ) ;

  QList<QGenericArgument> gargs;
  int k;
  for (k = 0; k < c.args.size(); ++k) {
    gargs << QGenericArgument(c.args[k].typeName(), c.args[k].data());
  }
  while (k++ < 9)
    gargs << QGenericArgument();
  bool r = QMetaObject::invokeMethod(target, c.slot.toLatin1(), gargs[0], gargs[1], gargs[2],
				     gargs[3], gargs[4], gargs[5], gargs[6], gargs[7], gargs[8]);
  if (!r) {
    qWarning()<<KLF_FUNC_NAME<<": Failed to execute command "<<url;
  }
}


