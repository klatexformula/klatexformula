/***************************************************************************
 *   file klfstyle.cpp
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

#include <QColor>

#include <klfutil.h>
#include <klfdatautil.h>

#include "klfstyle.h"


KLF_DECLARE_POBJ_TYPE(KLFStyle);



KLFStyle::BBoxExpand::BBoxExpand(double t, double r, double b, double l)
  : KLFPropertizedObject("KLFStyle::BBoxExpand"),
    top(this, Top, "top", t), right(this, Right, "right", r),
    bottom(this, Bottom, "bottom", b), left(this, Left, "left", l)
{
}

KLFStyle::BBoxExpand::BBoxExpand(const BBoxExpand& c)
  : KLFPropertizedObject("KLFStyle::BBoxExpand"),
    top(this, Top, "top", c.top), right(this, Right, "right", c.right),
    bottom(this, Bottom, "bottom", c.bottom), left(this, Left, "left", c.left)
{
}

// -----

// enum { Name, FgColor, BgColor, MathMode, Preamble, DPI, OverrideBBoxExpand, UserScript }

KLFStyle::KLFStyle(QString nm, unsigned long fgcol, unsigned long bgcol, const QString& mmode,
		   const QString& pre, int dotsperinch, const BBoxExpand& bb, const QString& us)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", nm), fg_color(this, FgColor, "fg_color", fgcol),
    bg_color(this, BgColor, "bg_color", bgcol), mathmode(this, MathMode, "mathmode", mmode),
    preamble(this, Preamble, "preamble", pre), dpi(this, DPI, "dpi", dotsperinch),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", bb),
    userScript(this, UserScript, "userScript", us)
{
}
KLFStyle::KLFStyle(const KLFBackend::klfInput& input)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", QString()), fg_color(this, FgColor, "fg_color", input.fg_color),
    bg_color(this, BgColor, "bg_color", input.bg_color), mathmode(this, MathMode, "mathmode", input.mathmode),
    preamble(this, Preamble, "preamble", input.preamble), dpi(this, DPI, "dpi", input.dpi),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", BBoxExpand()),
    userScript(this, UserScript, "userScript", input.userScript)
{
}
KLFStyle::KLFStyle(const KLFStyle& o)
  : KLFPropertizedObject("KLFStyle"),
    name(this, Name, "name", o.name), fg_color(this, FgColor, "fg_color", o.fg_color),
    bg_color(this, BgColor, "bg_color", o.bg_color), mathmode(this, MathMode, "mathmode", o.mathmode),
    preamble(this, Preamble, "preamble", o.preamble), dpi(this, DPI, "dpi", o.dpi),
    overrideBBoxExpand(this, OverrideBBoxExpand, "overrideBBoxExpand", o.overrideBBoxExpand),
    userScript(this, UserScript, "userScript", o.userScript)
{
}


QByteArray KLFStyle::typeNameFor(const QString& pname) const
{
  if (pname == "name")  return "QString";
  if (pname == "fg_color")  return "uint";
  if (pname == "bg_color")  return "uint";
  if (pname == "mathmode")  return "QString";
  if (pname == "preamble")  return "QString";
  if (pname == "dpi")  return "int";
  if (pname == "overrideBBoxExpand")  return "KLFStyle::BBoxExpand";
  if (pname == "userScript")  return "QString";
  qWarning()<<KLF_FUNC_NAME<<": Unknown property name "<<pname;
  return QByteArray();
}



KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle::BBoxExpand& b)
{
  return stream << b.top() << b.right() << b.bottom() << b.left();
}

KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle::BBoxExpand& x)
{
  double t, r, b, l;
  stream >> t >> r >> b >> l;
  x.top = t; x.right = r; x.bottom = b; x.left = l;
  return stream;
}


static QString preamble_append_overridebboxexpand(const KLFStyle::BBoxExpand& bb)
{
  if (!bb.valid())
    return QString();
  return "\n%%% KLF_overrideBBoxExpand: " + QString::fromLatin1(klfSave(&bb, QLatin1String("TextVariantMap")));
}
static QString preamble_append_userscript(const QString& us)
{
  if (us.isEmpty())
    return QString();
  return "\n%%% KLF_userScript: " + QString::fromLatin1(klfDataToEscaped(us.toUtf8()));
}

static void set_xtra_from_preamble(KLFStyle * style)
{
  QRegExp rx = QRegExp("\n%%%\\s*KLF_([a-zA-Z0-9_]*):\\s*(\\S[^\n]*)?");
  QString p = style->preamble;
  int pos = 0;
  while ((pos = rx.indexIn(p, pos)) != -1) {
    QString what = rx.cap(1);
    QString value = rx.cap(2);
    if (what == "overrideBBoxExpand") {
      KLFStyle::BBoxExpand bb;
      bool ok = klfLoad(value.toLatin1(), &bb);
      klfDbg("bbox value string: "<<value) ;
      KLF_ASSERT_CONDITION(ok, "Failed to read bbox expand info: "<<value, pos += rx.matchedLength(); continue; ) ;
      style->overrideBBoxExpand = bb;
      p.replace(pos, rx.matchedLength(), "\n");
      ++pos;
      continue;
    }
    if (what == "userScript") {
      style->userScript = QString::fromUtf8(klfEscapedToData(value.toLatin1()));
      klfDbg("read user script: "<<style->userScript()) ;
      p.replace(pos, rx.matchedLength(), "");
      continue;
    }
    qWarning()<<KLF_FUNC_NAME<<": Warning: ignoring unknown preamble-xtra-information "<<what ;
    pos += rx.matchedLength();
  }
  style->preamble = p; // with the xtra info removed
}


KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  // yes, QIODevice inherits QObject and we can use dynamic properties...
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    return stream << style.name() << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode()
		  << (style.preamble + "\n" +
		      preamble_append_overridebboxexpand(style.overrideBBoxExpand) +
		      preamble_append_userscript(style.userScript))
		  << (quint16)style.dpi;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    return stream << style.name() << (quint32)style.fg_color << (quint32)style.bg_color
		  << style.mathmode()
		  << (style.preamble + "\n" +
		      preamble_append_userscript(style.userScript))
		  << (quint16)style.dpi
		  << style.overrideBBoxExpand();
  } else {
    KLFStyle sty = style;
    // use Binary Properties saver. XML is useless, we're saving to binary stream
    QByteArray props = klfSave(&sty, QLatin1String("Binary"));
    klfDbg("got data: "<<props) ;
    // and save the data as a QByteArray
    return stream << props;
  }
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME+"(QDataStream,KLFStyle&)") ;
  QString compat_klfversion = stream.device()->property("klfDataStreamAppVersion").toString();
  if (klfVersionCompare(compat_klfversion, "3.1") <= 0) {
    quint32 fg, bg;
    quint16 dpi;
    QString name, mathmode, preamble;
    stream >> name;
    stream >> fg >> bg >> mathmode >> preamble >> dpi;
    style.name = name;
    style.mathmode = mathmode;
    style.preamble = preamble;
    style.fg_color = fg;
    style.bg_color = bg;
    style.dpi = dpi;
    style.overrideBBoxExpand = KLFStyle::BBoxExpand();
    style.userScript = QString();
    set_xtra_from_preamble(&style);
    return stream;
  } else if (klfVersionCompare(compat_klfversion, "3.3") < 0) {
    quint32 fg, bg;
    quint16 dpi;
    QString name, mathmode, preamble;
    KLFStyle::BBoxExpand bb;
    stream >> name;
    stream >> fg >> bg >> mathmode >> preamble >> dpi;
    stream >> bb;
    style.name = name;
    style.mathmode = mathmode;
    style.preamble = preamble;
    style.fg_color = fg;
    style.bg_color = bg;
    style.dpi = dpi;
    style.overrideBBoxExpand = bb;
    style.userScript = QString();
    set_xtra_from_preamble(&style);
    return stream;
  } else {
    // use Compressed XML
    QByteArray data;
    stream >> data;
    klfDbg("loading from data="<<data) ;
    bool loadOk = klfLoad(data, &style); // guessed format from magic header
    KLF_ASSERT_CONDITION(loadOk, "Failed to load style data", return stream; ) ;
    return stream;
  }
}

KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b)
{
  return a.name == b.name &&
    a.fg_color == b.fg_color &&
    a.bg_color == b.bg_color &&
    a.mathmode == b.mathmode &&
    a.preamble == b.preamble &&
    a.dpi == b.dpi &&
    a.overrideBBoxExpand == b.overrideBBoxExpand &&
    a.userScript == b.userScript;
}




