#include "HeatMapPlugin.h"

#include "PointData.h"
#include "ClusterData.h"
#include "event/Event.h"

#include <actions/PluginTriggerAction.h>

#include <QtCore>
#include <QtDebug>
#include <QWebEnginePage>

Q_PLUGIN_METADATA(IID "nl.tudelft.HeatMapPlugin")

using namespace hdps;

// =============================================================================
// View
// =============================================================================

HeatMapPlugin::HeatMapPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr)
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

    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(&_widget, "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
    _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
        gui::DropWidget::DropRegions dropRegions;
        
        const auto mimeText = mimeData->text();
        const auto tokens = mimeText.split("\n");

        if (tokens.count() == 1)
            return dropRegions;

        const auto datasetGuid = tokens[1];
        const auto dataType    = DataType(tokens[2]);
        const auto dataTypes   = DataTypes({ PointType, ClusterType });

        if (!dataTypes.contains(dataType))
            dropRegions << new gui::DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

        if (dataType == PointType) {
            const auto candidateDataset = _core->requestDataset<Points>(datasetGuid);
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

        if (dataType == ClusterType) {
            const auto candidateDataset = _core->requestDataset<Clusters>(datasetGuid);
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

    connect(_heatmap, SIGNAL(clusterSelectionChanged(QList<int>)), SLOT(clusterSelected(QList<int>)));
    connect(_heatmap, SIGNAL(dataSetPicked(QString)), SLOT(dataSetPicked(QString)));

    // Add widgets to plugin layout
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_heatmap);
    
	_widget.setLayout(layout);
}

void HeatMapPlugin::loadData(const hdps::Datasets& datasets)
{
    _datasetsDeferredLoad = datasets;

    _deferredLoadTimer.start();
}

void HeatMapPlugin::onDataEvent(hdps::DataEvent* dataEvent)
{
    // Event which gets triggered when a dataset is added to the system.
    if (dataEvent->getType() == EventType::DataAdded)
    {
        _heatmap->addDataOption(dataEvent->getDataset()->getGuiName());
    }
    // Event which gets triggered when the data contained in a dataset changes.
    if (dataEvent->getType() == EventType::DataChanged)
    {
        updateData();
    }
}

void HeatMapPlugin::dataSetPicked(const QString& name)
{
    qDebug() << "DATA PICKED IN HEATMAP";
    updateData();
}

void HeatMapPlugin::clusterSelected(QList<int> selectedClusters)
{
    qDebug() << "CLUSTER SELECTION";
    qDebug() << selectedClusters;
    
    auto pointSelection = _points->getSelection<Points>();
    auto selection      = _clusters->getSelection<Clusters>();

    pointSelection->indices.clear();

    int numClusters = _clusters->getClusters().size();
    for (int i = 0; i < numClusters; i++)
    {
        Cluster& cluster = _clusters->getClusters()[i];

        if (selectedClusters[i]) {
            pointSelection->indices.insert(pointSelection->indices.end(), cluster.getIndices().begin(), cluster.getIndices().end());
            _core->notifyDatasetSelectionChanged(_points);
        }
    }
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
    std::vector<QString> names;
    if (source->getDimensionNames().size() == source->getNumDimensions())
        names = source->getDimensionNames();

    _heatmap->setData(_clusters->getClusters(), names, numDimensions);
}

// =============================================================================
// Factory
// =============================================================================

QIcon HeatMapPluginFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("burn", color);
}

ViewPlugin* HeatMapPluginFactory::produce()
{
    return new HeatMapPlugin(this);
}

hdps::DataTypes HeatMapPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;
    supportedTypes.append(PointType);
    supportedTypes.append(ClusterType);
    return supportedTypes;
}

hdps::gui::PluginTriggerActions HeatMapPluginFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
{
	PluginTriggerActions pluginTriggerActions;

	const auto getPluginInstance = [this]() -> HeatMapPlugin* {
		return dynamic_cast<HeatMapPlugin*>(Application::core()->requestPlugin(getKind()));
	};

	const auto numberOfDatasets = datasets.count();

	if (numberOfDatasets == 2 && datasets[0]->getDataType() == PointType && datasets[1]->getDataType() == ClusterType) {
		auto pluginTriggerAction = createPluginTriggerAction("Heatmap", "View clusters in heatmap", datasets, "burn");

		connect(pluginTriggerAction, &QAction::triggered, [this, getPluginInstance, datasets]() -> void {
            getPluginInstance()->loadData(datasets);
        });

		pluginTriggerActions << pluginTriggerAction;
    }
    else {
		if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
			if (numberOfDatasets >= 1) {
				auto pluginTriggerAction = createPluginTriggerAction("Heatmap", "View clusters in heatmap", datasets, "burn");

				connect(pluginTriggerAction, &QAction::triggered, [this, getPluginInstance, datasets]() -> void {
					for (auto dataset : datasets)
						getPluginInstance()->loadData({ dataset });
                });

				pluginTriggerActions << pluginTriggerAction;
			}
		}
    }

	return pluginTriggerActions;
}
