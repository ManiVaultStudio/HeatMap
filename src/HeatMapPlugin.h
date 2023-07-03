#pragma once

#include <ViewPlugin.h>

#include "Dataset.h"

#include "HeatMapWidget.h"
#include "widgets/DropWidget.h"

#include <QList>
#include <QTimer>

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

    /**
     * Load one (or more datasets in the view)
     * @param datasets Dataset(s) to load
     */
    void loadData(const hdps::Datasets& datasets) override;

    void onDataEvent(hdps::DatasetEvent* dataEvent);
    
protected slots:
    void dataSetPicked(const QString& name);
    void clusterSelected(QList<int> selectedClusters);

private:
    void updateData();

    hdps::Datasets              _datasetsDeferredLoad;      /** Datasets cannot be loaded straight after the plugin is loaded because the web page needs to load first */
    QTimer                      _deferredLoadTimer;         /** Wait for the web page to load before loading the datasets */
    hdps::Dataset<Points>       _points;                    /** Currently loaded points dataset */
    hdps::Dataset<Clusters>     _clusters;                  /** Currently loaded clusters dataset */
    HeatMapWidget*              _heatmap;                   /** Heatmap widget displaying cluster data */
    hdps::gui::DropWidget*      _dropWidget;                /** Widget allowing users to drop in data */
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

    /**
     * Get plugin icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;
    
    ViewPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    hdps::gui::PluginTriggerActions getPluginTriggerActions(const hdps::Datasets& datasets) const override;
};
