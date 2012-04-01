/***************************************************************************
 *   file filterscript.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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


#ifndef PLUGINS_FILTERSCRIPT_H
#define PLUGINS_FILTERSCRIPT_H

#include <QtCore>
#include <QtGui>

#include <klfpluginiface.h>
#include <klfconfig.h>

#include <ui_filterscriptconfigwidget.h>




class FilterScriptConfigWidget : public QWidget, public Ui::FilterScriptConfigWidget
{
  Q_OBJECT
public:
  FilterScriptConfigWidget(QWidget *fitlerscriptconfigwidget, KLFPluginConfigAccess *conf);
  virtual ~FilterScriptConfigWidget() { }


  bool getModifiedAndReset() { bool m = _modified; _modified = false; return m; }

public slots:
  void loadSkinList(QString skin);
  void skinSelected(int index);
  void refreshSkin();

  void updateSkinDescription(const Skin& skin);

  void on_chkNoSyntaxHighlightingChange_toggled(bool /*value*/)
  { _modified = true; }

private:
  KLFPluginConfigAccess *config;

  bool _modified;
};



class FilterScriptPlugin : public QObject, public KLFPluginGenericInterface
{
  Q_OBJECT
  Q_INTERFACES(KLFPluginGenericInterface)
public:
  FilterScriptPlugin() : QObject(qApp) { }
  virtual ~FilterScriptPlugin() { }

  virtual QVariant pluginInfo(PluginInfo which) const {
    switch (which) {
    case PluginName: return QString("filterscript");
    case PluginAuthor: return QString("Philippe Faist <philippe.faist")+QString("@bluewin.ch>");
    case PluginTitle: return tr("Fitler Script");
    case PluginDescription: return tr("Apply scripts to equation images");
    case PluginDefaultEnable: return true;
    default:
      return QVariant();
    }
  }

  virtual void initialize(QApplication *app, KLFMainWin *mainWin, KLFPluginConfigAccess *config);

  virtual QWidget * createConfigWidget(QWidget *parent);
  virtual void loadFromConfig(QWidget *confwidget, KLFPluginConfigAccess *config);
  virtual void saveToConfig(QWidget *confwidget, KLFPluginConfigAccess *config);

  virtual Skin applySkin(KLFPluginConfigAccess *config, bool isStartup);

  virtual bool eventFilter(QObject *object, QEvent *event);

signals:
  void skinChanged(const QString& skin);

public slots:
  virtual void changeSkin(const QString& newSkin, bool force = false);
  virtual void changeSkinHelpLinkAction(const QUrl& link);

protected:
  KLFMainWin *_mainwin;
  QApplication *_app;

  KLFPluginConfigAccess *_config;

};






#endif
