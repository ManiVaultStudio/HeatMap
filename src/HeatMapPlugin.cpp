#include "HeatMapPlugin.h"

#include "PointData.h"
#include "ClusterData.h"

#include <QtCore>
#include <QtDebug>

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
}

HeatMapPlugin::~HeatMapPlugin(void)
{
    
}

void HeatMapPlugin::init()
{
    _heatmap->setPage(":/heatmap/heatmap.html", "qrc:/heatmap/");

    setDockingLocation(DockableWidget::DockingLocation::Right);
    setFocusPolicy(Qt::ClickFocus);

    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(this, "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
    _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
        gui::DropWidget::DropRegions dropRegions;
        
        const auto mimeText = mimeData->text();
        const auto tokens = mimeText.split("\n");

        if (tokens.count() == 1)
            return dropRegions;

        const auto datasetName = tokens[0];
        const auto dataType = DataType(tokens[1]);
        const auto dataTypes = DataTypes({ PointType, ClusterType });
        const auto currentDatasetName = _points.getDatasetName();

        if (!dataTypes.contains(dataType))
            dropRegions << new gui::DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", false);

        if (dataType == PointType) {
            const auto candidateDataset = _core->requestData<Points>(datasetName);
            const auto candidateDatasetName = candidateDataset.getName();
            const auto description = QString("Visualize %1 as points or density/contour map").arg(candidateDatasetName);

            if (!_points.isValid()) {
                dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, true, [this, candidateDatasetName]() {
                    _points.setDatasetName(candidateDatasetName);
                });
            }
            else {
                if (candidateDatasetName == currentDatasetName) {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Warning", "Data already loaded", false);
                }
                else {
                    const auto points = _core->requestData<Points>(currentDatasetName);

                    if (points.getNumPoints() != candidateDataset.getNumPoints()) {
                        dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, true, [this, candidateDatasetName]() {
                            _points.setDatasetName(candidateDatasetName);
                        });
                    }
                    else {
                        dropRegions << new gui::DropWidget::DropRegion(this, "Position", description, true, [this, candidateDatasetName]() {
                            _points.setDatasetName(candidateDatasetName);
                        });
                    }
                }
            }
        }

        if (dataType == ClusterType) {
            const auto candidateDataset = _core->requestData<Clusters>(datasetName);
            const auto candidateDatasetName = candidateDataset.getName();
            const auto description = QString("Clusters points by %1").arg(candidateDatasetName);

            if (_points.isValid()) {
                if (candidateDatasetName == _clusters.getDatasetName()) {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", "Cluster set is already in use", false, [this]() {});
                }
                else {
                    dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", description, true, [this, candidateDatasetName]() {
                        _clusters.setDatasetName(candidateDatasetName);
                    });
                }
            }
        }

        return dropRegions;
    });

    const auto updateWindowTitle = [this]() -> void {
        if (!_points.isValid())
            setWindowTitle(getGuiName());
        else
            setWindowTitle(QString("%1: %2").arg(getGuiName(), _points.getDatasetName()));
    };

    // Load points when the dataset name of the points dataset reference changes
    connect(&_points, &DatasetRef<Points>::datasetNameChanged, this, [this, updateWindowTitle](const QString& oldDatasetName, const QString& newDatasetName) {
        //loadPoints(newDatasetName);
        updateWindowTitle();
    });

    // Load clusters when the dataset name of the clusters dataset reference changes
    connect(&_clusters, &DatasetRef<Points>::datasetNameChanged, this, [this, updateWindowTitle](const QString& oldDatasetName, const QString& newDatasetName) {
        //loadPoints(newDatasetName);
        updateWindowTitle();
        updateData();
    });

    connect(_heatmap, SIGNAL(clusterSelectionChanged(QList<int>)), SLOT(clusterSelected(QList<int>)));
    connect(_heatmap, SIGNAL(dataSetPicked(QString)), SLOT(dataSetPicked(QString)));

    // Add widgets to plugin layout
    auto layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(_heatmap);
    setLayout(layout);
}

void HeatMapPlugin::onDataEvent(hdps::DataEvent* dataEvent)
{
    // Event which gets triggered when a dataset is added to the system.
    if (dataEvent->getType() == EventType::DataAdded)
    {
        _heatmap->addDataOption(dataEvent->dataSetName);
    }
    // Event which gets triggered when the data contained in a dataset changes.
    if (dataEvent->getType() == EventType::DataChanged)
    {
        updateData();
    }
    // Event which gets triggered when a dataset is removed from the system.
    if (dataEvent->getType() == EventType::DataRemoved)
    {
        // Request the point data that has been removed for further processing
        const Points& removedSet = _core->requestData<Points>(dataEvent->dataSetName);

        // ...
    }
    // Event which gets triggered when the selection associated with a dataset changes.
    if (dataEvent->getType() == EventType::SelectionChanged)
    {
        // Request the point data associated with the changed selection, and retrieve the selection from it
        const Points& changedDataSet = _core->requestData<Points>(dataEvent->dataSetName);
        const hdps::DataSet& selectionSet = changedDataSet.getSelection();

        // ...
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
    
    Points& pointSelection = static_cast<Points&>(_points->getSelection());
    Clusters& selection = static_cast<Clusters&>(_clusters->getSelection());
    pointSelection.indices.clear();

    int numClusters = _clusters->getClusters().size();
    for (int i = 0; i < numClusters; i++)
    {
        Cluster& cluster = _clusters->getClusters()[i];

        if (selectedClusters[i]) {
            pointSelection.indices.insert(pointSelection.indices.end(), cluster.getIndices().begin(), cluster.getIndices().end());
            _core->notifySelectionChanged(_points->getName());
        }
    }
}

void HeatMapPlugin::updateData()
{
    if (!_points.isValid() || !_clusters.isValid())
        return;

    Points& source = DataSet::getSourceData(*_points);

    qDebug() << "Working on data: " << _clusters->getName();
    
    qDebug() << "Calculating data";

    int numClusters = _clusters->getClusters().size();
    int numDimensions = 1;

    qDebug() << "Initialize clusters" << numClusters;
    // For every cluster initialize the median, mean, and stddev vectors with the number of dimensions
    for (int i = 0; i < numClusters; i++) {
        Cluster& cluster = _clusters->getClusters()[i];
                
        numDimensions = source.getNumDimensions();
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
                mean += source.getValueAt(index * numDimensions + d);

            mean /= cluster.getIndices().size();

            // Standard deviation calculation
            float variance = 0;

            for (int index : cluster.getIndices())
                variance += pow(source.getValueAt(index * numDimensions + d) - mean, 2);

            float stddev = sqrt(variance / cluster.getIndices().size());

            means[d] = mean;
            stddevs[d] = stddev;
        }
    }

    qDebug() << "Done calculating data";

    _heatmap->setData(_clusters->getClusters(), numDimensions);
}

// =============================================================================
// Factory
// =============================================================================

QIcon HeatMapPluginFactory::getIcon() const
{
    return Application::getIconFont("FontAwesome").getIcon("braille");
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
