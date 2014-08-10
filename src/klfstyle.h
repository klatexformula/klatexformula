/***************************************************************************
 *   file klfstyle.h
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

#ifndef KLFSTYLE_H
#define KLFSTYLE_H

#include <QList>
#include <QDataStream>

#include <klfdefs.h>
#include <klfdebug.h>
#include <klfbackend.h>
#include <klfpobj.h>

/** \brief A Formula Style (collection of properties)
 *
 * Structure containing forground color, bg color, mathmode, preamble, etc.
 */
struct KLF_EXPORT KLFStyle : public KLFPropertizedObject
{
  /** \brief a structure memorizing parameters for bbox expansion
   *
   * Stores how much to expand (EPS) BBox in each of top, right, bottom, and left directions,
   * in units of Postscript Points.
   *
   * BBox expansion is done in KLFBackend::getLatexFormula() to add margins on the sides of the
   * resulting image.
   *
   */
  struct KLF_EXPORT BBoxExpand : public KLFPropertizedObject
  {
    BBoxExpand(double t = -1, double r = -1, double b = -1, double l = -1);
    BBoxExpand(const BBoxExpand& c);

    /** Property IDs */
    enum { Top, Right, Bottom, Left };

    inline bool valid() const { return top() >= 0 && right() >= 0 && bottom() >= 0 && left() >= 0; }

    KLFPObjPropRef<double> top;
    KLFPObjPropRef<double> right;
    KLFPObjPropRef<double> bottom;
    KLFPObjPropRef<double> left;

    inline const BBoxExpand& operator=(const BBoxExpand& other)
    { top = other.top; right = other.right; bottom = other.bottom; left = other.left;  return *this; }
    inline bool operator==(const BBoxExpand& x) const
    { return top == x.top && right == x.right && bottom == x.bottom && left == x.left; }

    bool hasFixedTypes() const { return true; }
    QByteArray typeNameFor(const QString&) const { return "double"; }
  };

  enum { Name, FontName, FgColor, BgColor, MathMode, Preamble, FontSize, DPI, VectorScale,
	 OverrideBBoxExpand, UserScript, UserScriptInput };

  KLFStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	   unsigned long bgcol = qRgba(255,255,255,0), const QString& mmode = QString(),
	   const QString& pre = QString(), int dotsperinch = -1,
	   const BBoxExpand& bb = BBoxExpand(), const QString& us = QString(),
	   const QVariantMap& usinput = QVariantMap());

  KLFStyle(const KLFBackend::klfInput& input);

  KLFStyle(const KLFStyle& copy);

  KLFPObjPropRef<QString> name; ///< this may not always be set, it's only important in saved style list.
  KLFPObjPropRef<QString> fontname;
  KLFPObjPropRef<unsigned long> fg_color;
  KLFPObjPropRef<unsigned long> bg_color;
  KLFPObjPropRef<QString> mathmode;
  KLFPObjPropRef<QString> preamble;
  KLFPObjPropRef<double> fontsize;
  KLFPObjPropRef<int> dpi;
  KLFPObjPropRef<double> vectorscale;
  KLFPObjPropRef<BBoxExpand> overrideBBoxExpand;
  KLFPObjPropRef<QString> userScript;
  KLFPObjPropRef<QVariantMap> userScriptInput;

  inline const KLFStyle& operator=(const KLFStyle& o) {
    name = o.name; fontname = o.fontname; fg_color = o.fg_color; bg_color = o.bg_color;
    mathmode = o.mathmode; preamble = o.preamble; fontsize = o.fontsize; dpi = o.dpi;
    vectorscale = o.vectorscale;
    overrideBBoxExpand = o.overrideBBoxExpand;
    userScript = o.userScript;
    userScriptInput = o.userScriptInput;
    return *this;
  }

  bool hasFixedTypes() const { return true; }
  QByteArray typeNameFor(const QString&) const;



  /** Style structure, as was in KLF 3.1 (for which we are keeping backwards
   * compatibility)
   *
   * This is simple and straightforward. The more sophisticated and advanced newer
   * KLFStyle, when converted to a KLFLegacyStyle (see \ref fromNewStyle()), stores all the
   * extra, newer, properties, as LaTeX comments in the preamble.
   */
  struct KLFLegacyStyle {
    KLFLegacyStyle(QString nm = QString(), unsigned long fgcol = qRgba(0,0,0,255),
	     unsigned long bgcol = qRgba(255,255,255,0),
	     const QString& mmode = QString(),
	     const QString& pre = QString(),
	     int dotsperinch = -1)
      : name(nm), fg_color(fgcol), bg_color(bgcol), mathmode(mmode), preamble(pre),
	dpi(dotsperinch) { }
    QString name; // this may not always be set, it's only important in saved style list.
    unsigned long fg_color;
    unsigned long bg_color;
    QString mathmode;
    QString preamble;
    int dpi;

    KLFStyle toNewStyle() const;
    static KLFLegacyStyle fromNewStyle(const KLFStyle& s);
  };

};

Q_DECLARE_METATYPE(KLFStyle) ;
Q_DECLARE_METATYPE(KLFStyle::BBoxExpand) ;

typedef QList<KLFStyle> KLFStyleList;

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle& style);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle& style);
// exact matches
KLF_EXPORT bool operator==(const KLFStyle& a, const KLFStyle& b);


// legacy style
KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFStyle::KLFLegacyStyle& style);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFStyle::KLFLegacyStyle& style);
KLF_EXPORT bool operator==(const KLFStyle::KLFLegacyStyle& a, const KLFStyle::KLFLegacyStyle& b);

KLF_EXPORT QDebug& operator<<(QDebug& stream, const KLFStyle::KLFLegacyStyle& style);



#endif
