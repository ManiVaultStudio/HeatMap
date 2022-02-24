#pragma once

#include <ViewPlugin.h>

#include "Dataset.h"

#include "HeatMapWidget.h"
#include "widgets/DropWidget.h"

#include <QList>

using namespace hdps::plugin;
using namespace hdps::util;

class Points;
class Clusters;

// =============================================================================
// View
// =============================================================================

/**
 * Heatmap Plugin
 *
 * This plugin visualizes clusters belonging to some dataset by showing their cluster
 * statistics and coloring them accordingly. It allows selecting clusters in an intuitive
 * manner with linked selection to the original dataset.
 *
 * @author Julian Thijssen
 */
class HeatMapPlugin : public ViewPlugin
{
    Q_OBJECT
    
public:
    HeatMapPlugin(const PluginFactory* factory);
    ~HeatMapPlugin(void) override;
    
    void init() override;

    void onDataEvent(hdps::DataEvent* dataEvent);
    
protected slots:
    void dataSetPicked(const QString& name);
    void clusterSelected(QList<int> selectedClusters);

private:
    void updateData();

    hdps::Dataset<Points>              _points;        /** Currently loaded points dataset */
    hdps::Dataset<Clusters>            _clusters;      /** Currently loaded clusters dataset */

    HeatMapWidget*                  _heatmap;       /** Heatmap widget displaying cluster data */
    hdps::gui::DropWidget*          _dropWidget;    /** Widget allowing users to drop in data */
};

// =============================================================================
// Factory
// =============================================================================

class HeatMapPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(hdps::plugin::ViewPluginFactory hdps::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.tudelft.HeatMapPlugin"
                      FILE  "HeatMapPlugin.json")
    
public:
    HeatMapPluginFactory(void) {}
    ~HeatMapPluginFactory(void) override {}

    /** Returns the plugin icon */
    QIcon getIcon() const override;
    
    ViewPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;
};
