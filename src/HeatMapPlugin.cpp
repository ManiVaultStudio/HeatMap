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

    auto layout = new QVBoxLayout();

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(_heatmap);

    setLayout(layout);

    connect(_heatmap, SIGNAL(clusterSelectionChanged(QList<int>)), SLOT(clusterSelected(QList<int>)));
    connect(_heatmap, SIGNAL(dataSetPicked(QString)), SLOT(dataSetPicked(QString)));
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
    
    Clusters& clusters = _core->requestData<Clusters>(_heatmap->getCurrentData());
    Clusters& selection = static_cast<Clusters&>(clusters.getSelection());
    selection.indices.clear();

    int numClusters = clusters.getClusters().size();
    for (int i = 0; i < numClusters; i++)
    {
        Cluster& cluster = clusters.getClusters()[i];

        if (selectedClusters[i]) {
            selection.indices.insert(selection.indices.end(), cluster.getIndices().begin(), cluster.getIndices().end());
            _core->notifySelectionChanged(clusters.getName());
        }
    }
}

void HeatMapPlugin::updateData()
{
    if (!_points.isValid())
        return;

    QString currentData = _heatmap->getCurrentData();

    qDebug() << "Working on data: " << currentData;
    qDebug() << "Attempting cast to ClusterSet";
    Clusters& clusterSet = _core->requestData<Clusters>(currentData);
    
    qDebug() << "Calculating data";

    int numClusters = clusterSet.getClusters().size();
    int numDimensions = 1;

    std::vector<Cluster> clusters;
    clusters.resize(numClusters);
    qDebug() << "Initialize clusters" << numClusters;
    // For every cluster initialize the median, mean, and stddev vectors with the number of dimensions
    for (int i = 0; i < numClusters; i++) {
        Cluster& cluster = clusterSet.getClusters()[i];
                
        numDimensions = _points->getNumDimensions();
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
                mean += _points->getValueAt(index * numDimensions + d);

            mean /= cluster.getIndices().size();

            // Standard deviation calculation
            float variance = 0;

            for (int index : cluster.getIndices())
                variance += pow(_points->getValueAt(index * numDimensions + d) - mean, 2);

            float stddev = sqrt(variance / cluster.getIndices().size());

            means[d] = mean;
            stddevs[d] = stddev;
        }
    }

    qDebug() << "Done calculating data";

    _heatmap->setData(clusters, numDimensions);
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
