/***************************************************************************
 *   file plugins/skin/skin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2009 by Philippe Faist
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

#ifndef PLUGINS_SKIN_H
#define PLUGINS_SKIN_H

#include <QtGui>

#include <klfpluginiface.h>
#include <klfconfig.h>

#include <ui_skinconfigwidget.h>

class SkinConfigWidget : public QWidget, public Ui::SkinConfigWidget
{
  Q_OBJECT
public:
  SkinConfigWidget(QWidget *skinconfigwidget, KLFPluginConfigAccess *conf);
  virtual ~SkinConfigWidget() { }

  QString currentSkin() { return _skins[cbxSkin->itemData(cbxSkin->currentIndex()).toInt()].name; }
  QString currentStyleSheet() { return txtStyleSheet->toPlainText(); }

public slots:
  void load(QString skin, QString stylesheet);
  void skinSelected(int index);
  void stylesheetChanged();
  void saveCustom();

private:
  struct Skin {
    Skin(bool b, const QString& nm, const QString& ttl, const QString& ssheet)
      : builtin(b), name(nm), title(ttl), stylesheet(ssheet)  {  }
    Skin(const Skin& o) : builtin(o.builtin), name(o.name), title(o.title), stylesheet(o.stylesheet) { }
    ~Skin() { }

    bool builtin;
    QString name;
    QString title;
    QString stylesheet;
  };
  QList<Skin> _skins;

  KLFPluginConfigAccess *config;
};

class SkinPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  virtual ~SkinPlugin() { }

  virtual QString pluginName() const { return "skin"; }
  virtual QString pluginTitle() const { return tr("Skin"); }
  virtual QString pluginDescription() const { return tr("Personalize the look of KLatexFormula"); }
  virtual bool pluginDefaultLoadEnable() const { return false; }

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config);

  virtual QWidget * createConfigWidget(QWidget *parent);
  virtual void loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config);
  virtual void saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config);

  virtual void applySkin(KLFPluginConfigAccess *config);

protected:
  KLFMainWin *_mainwin;

  KLFPluginConfigAccess *_config;
};


#endif
