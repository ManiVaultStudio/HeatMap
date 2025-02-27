#include "HeatMapPlugin.h"

#include <DatasetsMimeData.h>
#include <event/Event.h>

#include <ClusterData/ClusterData.h>
#include <PointData/PointData.h>

#include <actions/PluginTriggerAction.h>

#include <QtCore>
#include <QtDebug>
#include <QWebEnginePage>

Q_PLUGIN_METADATA(IID "studio.manivault.HeatMapPlugin")

using namespace mv;
using namespace mv::gui;

// =============================================================================
// View
// =============================================================================

HeatMapPlugin::HeatMapPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _datasetsDeferredLoad(),
    _deferredLoadTimer(),
    _points(),
    _clusters()
{
    _heatmap = new HeatMapWidget();
    _dropWidget = new gui::DropWidget(_heatmap);

    _deferredLoadTimer.setInterval(250);

    connect(&_deferredLoadTimer, &QTimer::timeout, this, [this]() -> void {
        if (_heatmap->getPage()->isLoading())
            return;

        _deferredLoadTimer.stop();

		if (_datasetsDeferredLoad.count() >= 1 && _datasetsDeferredLoad.first()->getDataType() != PointType)
			return;

		_points = Dataset<Points>(_datasetsDeferredLoad.first());

		if (_datasetsDeferredLoad.count() == 2 && _datasetsDeferredLoad[1]()->getDataType() == ClusterType)
			_clusters = Dataset<Clusters>(_datasetsDeferredLoad[1]);
    });
}

HeatMapPlugin::~HeatMapPlugin(void)
{
    
}

void HeatMapPlugin::init()
{
    _heatmap->setPage(":/heatmap/heatmap.html", "qrc:/heatmap/");

    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "First, drag a point data set and then a cluster data set from the data hierarchy here..."));
    _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
        gui::DropWidget::DropRegions dropRegions;
        
        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        const auto dataset  = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->getGuiName();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();
        const auto dataTypes   = DataTypes({ PointType, ClusterType });

        if (!dataTypes.contains(dataType))
            dropRegions << new gui::DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

        if (dataType == PointType) {
            const auto candidateDataset = mv::data().getDataset<Points>(datasetId);
            const auto candidateDatasetName = candidateDataset->getGuiName();
            const auto description = QString("Visualize %1 as points or density/contour map").arg(candidateDatasetName);

            if (!_points.isValid()) {
                dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, "map-marker-alt", true, [this, candidateDataset]() {
                    _points = candidateDataset;
                });
            }
            else {
                if (candidateDataset == _points) {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    if (_points->getNumPoints() != candidateDataset->getNumPoints()) {
                        dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, "map-marker-alt", true, [this, candidateDataset]() {
                            _points = candidateDataset;
                        });
                    }
                    else {
                        dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, "map-marker-alt", true, [this, candidateDataset]() {
                            _points = candidateDataset;
                        });
                    }
                }
            }
        }
        else if (dataType == ClusterType) {
            const auto candidateDataset = mv::data().getDataset<Clusters>(datasetId);
            const auto description      = QString("Clusters points by %1").arg(candidateDataset->getGuiName());

            if (_points.isValid()) {
                if (candidateDataset == _clusters) {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", "Cluster set is already in use", "exclamation-circle", false, [this]() {});
                }
                else {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", description, "th-large", true, [this, candidateDataset]() {
                        _clusters = candidateDataset;
                    });
                }
            }
        }

        return dropRegions;
    });

    const auto updateWindowTitle = [this]() -> void {
        if (!_points.isValid())
			_heatmap->setWindowTitle(getGuiName());
        else
			_heatmap->setWindowTitle(QString("%1: %2").arg(getGuiName(), _points->getGuiName()));
    };

    // Load points when the dataset name of the points dataset reference changes
    connect(&_points, &Dataset<Points>::changed, this, [this, updateWindowTitle]() {
        //loadPoints(newDatasetName);
        _dropWidget->setShowDropIndicator(false);
        updateWindowTitle();
    });

    // Load clusters when the dataset name of the clusters dataset reference changes
    connect(&_clusters, &Dataset<Clusters>::changed, this, [this, updateWindowTitle]() {
        //loadPoints(newDatasetName);
        updateWindowTitle();
        updateData();
        });

    // Load clusters when the dataset name of the clusters dataset reference changes
    connect(&_clusters, &Dataset<Clusters>::dataChanged, this, [this, updateWindowTitle]() {
        updateData();
    });

    // Load clusters when the dataset name of the clusters dataset reference changes
    connect(&_clusters, &Dataset<Clusters>::dataSelectionChanged, this, &HeatMapPlugin::selectClusters);

    connect(_heatmap, &HeatMapWidget::clusterSelectionChanged, this, &HeatMapPlugin::clusterSelected);
    connect(_heatmap, &HeatMapWidget::dataSetPicked, this, &HeatMapPlugin::dataSetPicked);

    // Add widgets to plugin layout
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_heatmap);
    
    getWidget().setLayout(layout);
}

void HeatMapPlugin::loadData(const mv::Datasets& datasets)
{
    _datasetsDeferredLoad = datasets;

    _deferredLoadTimer.start();
}

// TODO: remove this, it is not connected and does nothing
void HeatMapPlugin::onDataEvent(mv::DatasetEvent* dataEvent)
{
    // Event which gets triggered when a dataset is added to the system.
    if (dataEvent->getType() == EventType::DatasetAdded)
    {
        _heatmap->addDataOption(dataEvent->getDataset()->getGuiName());
    }
    // Event which gets triggered when the data contained in a dataset changes.
    if (dataEvent->getType() == EventType::DatasetDataChanged)
    {
        updateData();
    }
}

void HeatMapPlugin::dataSetPicked(const QString& name)
{
    updateData();
}

void HeatMapPlugin::clusterSelected(const std::vector<std::uint32_t>& selectedClusters)
{
    _clusters->setSelectionIndices(selectedClusters);
    events().notifyDatasetDataSelectionChanged(_clusters);
}

void HeatMapPlugin::selectClusters()
{
    const auto& selectionIndices = _clusters->getSelectionIndices();

    QList<int> selection(_clusters->getClusters().size(), 0);
    for (const auto& selectionIndex : selectionIndices)
        selection[selectionIndex] = 1;

    _heatmap->setSelection(selection);
}

void HeatMapPlugin::updateData()
{
    if (!_points.isValid() || !_clusters.isValid())
        return;

    auto source = _points->getSourceDataset<Points>();

    qDebug() << "Working on data: " << _clusters->getGuiName();
    
    qDebug() << "Calculating data";

    int numClusters = _clusters->getClusters().size();
    int numDimensions = 1;

    qDebug() << "Initialize clusters" << numClusters;
    // For every cluster initialize the median, mean, and stddev vectors with the number of dimensions
    for (int i = 0; i < numClusters; i++) {
        Cluster& cluster = _clusters->getClusters()[i];
                
        numDimensions = source->getNumDimensions();
        qDebug() << "Num dimensions: " << numDimensions;

        // Cluster statistics
        auto& means = cluster.getMean();
        auto& stddevs = cluster.getStandardDeviation();

        means.resize(numDimensions);
        stddevs.resize(numDimensions);

        for (int d = 0; d < numDimensions; d++)
        {
            // Mean calculation
            float mean = 0;

            for (int index : cluster.getIndices())
                mean += source->getValueAt(index * numDimensions + d);

            mean /= cluster.getIndices().size();

            // Standard deviation calculation
            float variance = 0;

            for (int index : cluster.getIndices())
                variance += pow(source->getValueAt(index * numDimensions + d) - mean, 2);

            float stddev = sqrt(variance / cluster.getIndices().size());

            means[d] = mean;
            stddevs[d] = stddev;
        }
    }

    qDebug() << "Done calculating data";
    std::vector<QString> dimensionNames;
    if (source->getDimensionNames().size() == source->getNumDimensions())
        dimensionNames = source->getDimensionNames();

    std::vector<QString> clusterNames = _clusters->getClusterNames();

    _heatmap->setData(_clusters->getClusters(), dimensionNames, clusterNames, numDimensions);
}

// =============================================================================
// Factory
// =============================================================================

HeatMapPluginFactory::HeatMapPluginFactory()
{}

ViewPlugin* HeatMapPluginFactory::produce()
{
    return new HeatMapPlugin(this);
}

mv::DataTypes HeatMapPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;
    supportedTypes.append(PointType);
    supportedTypes.append(ClusterType);
    return supportedTypes;
}

PluginTriggerActions HeatMapPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
	PluginTriggerActions pluginTriggerActions;

	const auto getPluginInstance = [this]() -> HeatMapPlugin* {
		return dynamic_cast<HeatMapPlugin*>(plugins().requestViewPlugin(getKind()));
	};

	const auto numberOfDatasets = datasets.count();

	if (numberOfDatasets == 2 && datasets[0]->getDataType() == PointType && datasets[1]->getDataType() == ClusterType) {
		auto pluginTriggerAction = new PluginTriggerAction(const_cast<HeatMapPluginFactory*>(this), this, "Heatmap", "View clusters in heatmap", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            getPluginInstance()->loadData(datasets);
        });

		pluginTriggerActions << pluginTriggerAction;
    }
    else {
		if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
			if (numberOfDatasets >= 1) {
				auto pluginTriggerAction = new PluginTriggerAction(const_cast<HeatMapPluginFactory*>(this), this, "Heatmap", "View clusters in heatmap", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
					for (auto dataset : datasets)
						getPluginInstance()->loadData({ dataset });
                });

				pluginTriggerActions << pluginTriggerAction;
			}
		}
    }

	return pluginTriggerActions;
}
