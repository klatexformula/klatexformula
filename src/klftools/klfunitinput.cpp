/***************************************************************************
 *   file klfunitinput.cpp
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

#include <math.h>

#include <QDebug>
#include <QVariant>
#include <QStringList>
#include <QComboBox>
#include <QDoubleSpinBox>

#include "klfunitinput.h"


KLFUnitChooser::KLFUnitChooser(QWidget *parent)
  : QComboBox(parent)
{
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}
KLFUnitChooser::~KLFUnitChooser()
{
}

void KLFUnitChooser::setUnits(const QStringList& unitstrlist)
{
  QList<Unit> units;
  int k;
  for (k = 0; k < unitstrlist.size(); ++k) {
    QStringList parts = unitstrlist[k].split('=');
    if (parts.size() != 3) {
      qWarning()<<KLF_FUNC_NAME<<": Invalid unit specification: "<<unitstrlist[k];
      continue;
    }
    Unit u;
    u.name = parts[0];
    u.abbrev = parts[1];
    u.factor = parts[2].toDouble();
    units << u;
  }
  setUnitList(units);
}

void KLFUnitChooser::setUnitList(const QList<Unit>& unitlist)
{
  clear(); // clear combo box
  pUnits = unitlist;
  int k;
  for (k = 0; k < pUnits.size(); ++k) {
    Unit u = pUnits[k];
    // add this cbx item
    addItem(u.name, QVariant::fromValue<Unit>(u));
  }
}

QStringList KLFUnitChooser::unitStrList() const
{
  QStringList l;
  int k;
  for (k = 0; k < pUnits.size(); ++k)
    l << QString("%2=%3=%1").arg(pUnits[k].factor, 0, 'g').arg(pUnits[k].name, pUnits[k].abbrev);
  return l;
}

void KLFUnitChooser::setCurrentUnit(const QString& unitname)
{
  int k;
  for (k = 0; k < count(); ++k) {
    if (itemData(k).value<Unit>().name == unitname) {
      setCurrentIndex(k);
      return;
    }
  }
  qWarning()<<KLF_FUNC_NAME<<": unit "<<unitname<<" not found.";
}

void KLFUnitChooser::internalCurrentIndexChanged(int index)
{
  Unit u = itemData(index).value<Unit>();
  emit unitChanged(u.name);
  emit unitChanged(u.factor);
}


// -------------------------


KLFUnitSpinBox::KLFUnitSpinBox(QWidget *parent)
  : QDoubleSpinBox(parent)
{
  pUnitFactor = 1.0f;
  connect(this, SIGNAL(valueChanged(double)), this, SLOT(internalValueChanged(double)));
}
KLFUnitSpinBox::~KLFUnitSpinBox()
{
}

void KLFUnitSpinBox::setUnit(double unitfactor)
{
  double curValue = value();
  double curMinimum = minimum();
  double curMaximum = maximum();
  double curUnitFactor = pUnitFactor;
  int curPrecision = decimals();

  klfDbg("unitfactor="<<unitfactor<<" cur: val="<<curValue<<",min="<<curMinimum<<",max="<<curMaximum
	 <<",prec="<<curPrecision) ;

  pUnitFactor = unitfactor;

  // calculate the right number of decimal places.
  // due to round-off errors, we need to first determine how many decimals we started
  // off with, then re-calculate the number of decimals.
  int unitRefDecimals = curPrecision - (int)( log((double)curUnitFactor)/log((double)10.0)  + 0.5 );

  setDecimals(unitRefDecimals + (int)( log((double)pUnitFactor)/log((double)10.0) + 0.5 ) );

  // set the appropriate range
  setMinimum(curMinimum * curUnitFactor / pUnitFactor);
  setMaximum(curMaximum * curUnitFactor / pUnitFactor);

  // and set the value
  setValue(curValue * curUnitFactor / pUnitFactor);
}

void KLFUnitSpinBox::setValueInRefUnit(double value)
{
  setValue(value / pUnitFactor);
}


void KLFUnitSpinBox::internalValueChanged(double valueInExtUnits)
{
  klfDbg("val in ext. units="<<valueInExtUnits<<"; our unitfactor="<<pUnitFactor) ;
  emit valueInRefUnitChanged(valueInExtUnits / pUnitFactor);
}




