/***************************************************************************
 *   file klfunitinput.h
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

#ifndef KLFUNITINPUT_H
#define KLFUNITINPUT_H

#include <QWidget>
#include <QString>
#include <QComboBox>
#include <QDoubleSpinBox>

#include <klfdefs.h>

/** \brief A combo box to select a unit for measures
 *
 * Typical usage:
 * - set possible units with setUnitList() or setUnits() or by setting the klfUnits property
 *   in Qt designer to a string-list as documented in setUnits()
 * - eg. connect signal unitChanged(double) to a KLFUnitSpinBox' setUnit(double) to allow us
 *   to change that spin box' unit; or set/retrieve manually the selected unit with the various
 *   getters (currentUnit(), currentUnitName(), etc.) and the setter setCurrentUnit().
 */
class KLF_EXPORT KLFUnitChooser : public QComboBox
{
  Q_OBJECT

  Q_PROPERTY(QString currentUnit READ currentUnitName WRITE setCurrentUnit USER true)
  Q_PROPERTY(double currentUnitFactor READ currentUnitFactor)
  Q_PROPERTY(QString klfUnits READ unitStringDescription WRITE setUnits)
public:
  KLFUnitChooser(QWidget *parent = NULL);
  virtual ~KLFUnitChooser();

  struct Unit {
    QString name;
    QString abbrev;
    double factor;
  };

  inline Unit currentUnit() const { return itemData(currentIndex()).value<Unit>(); }
  inline QString currentUnitName() const { return currentUnit().name; }
  inline QString currentUnitAbbrev() const { return currentUnit().abbrev; }
  inline double currentUnitFactor() const { return currentUnit().factor; }

  inline QStringList unitNames() const
  { QStringList l; foreach (Unit unit, pUnits) { l << unit.name; } return l;  }
  inline QList<Unit> unitList() const { return pUnits; }

  QString unitStringDescription() const;

public slots:
  /** Set the possible units user can choose from.
   * Units are specified as a string of semicolon-separated items, each item in the list corresponding
   * to one unit, specified as a string like \c "Inch=in=25.4" or \c "Centimeter=cm=10" or \c "Millimeter=mm=1", that
   * is a string with three sections separated by an \c '=' sign giving unit name, unit abbreviation,
   * and the factor of that unit to a reference unit. See KLFUnitSpinBox for discussion about units.
   *
   * Example:
   * \code
   * setUnits("Postscript Point=pt=1;Millimeter=mm=2.835;Centimeter=cm=28.35;1/8 th inch=1/8 in=9;Inch=in=72")
   * \endcode
   */
  void setUnits(const QString& unitstrlist);
  /** Set the possible units user can choose from. */
  void setUnits(const QList<Unit>& unitlist);

  void setCurrentUnit(const QString& unitName);
  void setCurrentUnitAbbrev(const QString& unitAbbrev);
  void setCurrentUnitIndex(int k);

signals:
  void unitChanged(const QString& unitName);
  void unitChanged(double unitFactor);
  void unitChanged(double unitFactor, const QString& suffix);

protected:
  virtual void changeEvent(QEvent *event);

private:
  QList<Unit> pUnits;

  QString pDelayedUnitSet;

private slots:
  void internalCurrentIndexChanged(int index);
};

Q_DECLARE_METATYPE(KLFUnitChooser::Unit) ;


/** \brief A spin box that can display values in different units
 *
 * This widget presents a spin box which displays a value, which by default is shown
 * in the 'reference unit', for which the 'unit factor' is \c 1.
 *
 * Other units may be set (eg. by connecting our setUnit(double) slot to the unitChanged(double)
 * signal of a KLFUnitChooser) which have other unit factors telling how to convert the other
 * value into the 'ref unit' value.
 *
 * Example:
 * \code
 *   // as the programmer, we have only to think which units the _program_ requests and define
 *   // that value to be the 'reference unit'. We assume for this example that the (fictive)
 *   // function  perform_adjustment(double)  requests an argument that is a length in millimeters.
 *   KLFUnitSpinBox spn = new KLFUnitSpinBox;
 *   spn->setValue(18); // We display the value 18 ref-units. (= mm here)
 *   spn->setUnit(25.4); // set a unit that is 25.4 'ref-units': will now display the value 0.709
 *   //                     which is 18mm in inches.
 *   ...
 *   // other units may be set with spn->setUnit(double unitfactor)
 *   ...
 *   // at the end, retrieve the value
 *   double valueInMM = spn->valueInRefUnit();
 *   perform_adjustment(valueInMM);
 * \endcode
 *
 * When units are changed, the minimum and maximum are adjusted to the value in the new units.
 *
 * The precision ( QDoubleSpinBox::setDecimals()) is also adjusted to the right order of magnitude
 * (power of 10). For example, if the value of 18.3mm is displayed with 1 decimal place, and a unit is
 * set with factor of \c 1000 (m) then the presision is adjusted to 4 decimal places. And if a unit is
 * set with factor of \c 304.8 (foot) then the precision is adjusted to 3 decimal places.
 * <i>Details: the number of decimal places to adjust is determined by rounding the value
 * <tt>log_10(unit-factor)</tt> to the nearest integer.</i>
 *
 * \note 'unit-factor' of unit \a XYZ is defined as the number of ref units needed to amount to one
 *   \a XYZ.
 */
class KLF_EXPORT KLFUnitSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
  Q_PROPERTY(double valurInRefUnit READ valueInRefUnit WRITE setValueInRefUnit USER true)
  Q_PROPERTY(double unitFactor READ unitFactor WRITE setUnit)
  Q_PROPERTY(bool showUnitSuffix READ showUnitSuffix WRITE setShowUnitSuffix)
public:
  KLFUnitSpinBox(QWidget *parent = NULL);
  virtual ~KLFUnitSpinBox();

  inline double unitFactor() const { return pUnitFactor; }

  inline bool showUnitSuffix() const { return pShowUnitSuffix; }

  inline double valueInRefUnit() const { return QDoubleSpinBox::value() * unitFactor(); }

signals:
  void valueInRefUnitChanged(double value);

public slots:
  void setUnit(double unitfactor);

  /** Display the current value converted to the new unit using \c unitfactor, and if showUnitSuffix() is
   * TRUE, then displays the given suffix.
   */
  void setUnitWithSuffix(double unitfactor, const QString& suffix);

  /** Whether to display the unit suffix or not. This only works if you use setUnitWithSuffix() to set
   * new units. */
  void setShowUnitSuffix(bool show);

  void setValueInRefUnit(double value);

private:
  double pUnitFactor;
  bool pShowUnitSuffix;

private slots:
  void internalValueChanged(double valueInExtUnits);
};



#endif
