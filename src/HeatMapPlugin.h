#pragma once

#include <ViewPlugin.h>

#include "Dataset.h"

#include "HeatMapWidget.h"
#include "widgets/DropWidget.h"

#include <QList>
#include <QTimer>

using namespace mv::plugin;
using namespace mv::util;

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

    /**
     * Load one (or more datasets in the view)
     * @param datasets Dataset(s) to load
     */
    void loadData(const mv::Datasets& datasets) override;

    // TODO: remove this, it is not connected and does nothing
    void onDataEvent(mv::DatasetEvent* dataEvent);
    
protected slots:
    void dataSetPicked(const QString& name);
    void clusterSelected(const std::vector<std::uint32_t>& selectedClusters);
    void selectClusters();

private:
    void updateData();

    mv::Datasets                _datasetsDeferredLoad;      /** Datasets cannot be loaded straight after the plugin is loaded because the web page needs to load first */
    QTimer                      _deferredLoadTimer;         /** Wait for the web page to load before loading the datasets */
    mv::Dataset<Points>         _points;                    /** Currently loaded points dataset */
    mv::Dataset<Clusters>       _clusters;                  /** Currently loaded clusters dataset */
    HeatMapWidget*              _heatmap;                   /** Heatmap widget displaying cluster data */
    mv::gui::DropWidget*        _dropWidget;                /** Widget allowing users to drop in data */
};

// =============================================================================
// Factory
// =============================================================================

class HeatMapPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.HeatMapPlugin"
                      FILE  "HeatMapPlugin.json")
    
public:
    HeatMapPluginFactory();

    ~HeatMapPluginFactory() override {}

    ViewPlugin* produce() override;

    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
