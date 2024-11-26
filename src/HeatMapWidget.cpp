#include "HeatMapWidget.h"

#include "ClusterData/ClusterData.h"

#include <cassert>

HeatMapCommunicationObject::HeatMapCommunicationObject(HeatMapWidget* parent) :
    _parent(parent)
{

}

void HeatMapCommunicationObject::js_selectData(QString text)
{
    _parent->js_selectData(text);
}

void HeatMapCommunicationObject::js_selectionUpdated(const QVariantList& selectedClusters)
{
    _parent->js_selectionUpdated(selectedClusters);
}

HeatMapWidget::HeatMapWidget() :
    mv::gui::WebWidget(),
    _communicationObject(nullptr),
    loaded(false),
    _numClusters(0),
    dataOptionBuffer()
{
    Q_INIT_RESOURCE(heatmap_resources);
    _communicationObject = new HeatMapCommunicationObject(this);
    init(_communicationObject);

    setAcceptDrops(true);
    setMouseTracking(true);
}

HeatMapWidget::~HeatMapWidget()
{

}

// TODO: remove this, it is not connected and does nothing
void HeatMapWidget::addDataOption(const QString option)
{
    if (loaded)
        emit _communicationObject->qt_addAvailableData(option);
    else
        dataOptionBuffer.append(option);
}

void HeatMapWidget::setData(const QVector<Cluster>& clusters, const std::vector<QString>& dimNames, const std::vector<QString>& clusterNames, const int numDimensions)
{
    std::string _jsonObject = "";

    _numClusters = clusters.size();
    qDebug() << "Setting data";
    //// Nodes
    std::string nodes = "\"nodes\":[\n";
    for (int i = 0; i < _numClusters; i++)
    {
        if(clusterNames.size() == _numClusters)
            nodes = nodes + "{\"name\":\"" + clusterNames[i].toStdString() + "\", ";
        else
            nodes = nodes + "{\"name\":\"" + "Cluster name " + std::to_string(i) + "\", ";
        nodes = nodes + "\"size\":" + std::to_string(clusters[i].getNumberOfIndices()) + ", ";
        nodes = nodes + "\"expression\":[";

        for (int j = 0; j < numDimensions; j++)
        {
            nodes = nodes + std::to_string(clusters[i].getMean()[j]);

            if (j < numDimensions - 1) nodes = nodes + ",";
        }
        nodes = nodes + "], \"stddev\":[";
        for (int j = 0; j < numDimensions; j++)
        {
            nodes = nodes + std::to_string(clusters[i].getStandardDeviation()[j]);

            if (j < numDimensions - 1) nodes = nodes + ",";
        }
        if (i < _numClusters - 1) nodes = nodes + "]},\n";
        else nodes = nodes + "]}\n]";
    }

    // TODO: multi files
    std::string names = ",\n\"names\":[";
    for (int i = 0; i < numDimensions; i++) {
        if (dimNames.size() > i)
            names = names + "\"" + dimNames[i].toStdString() + "\"";
        else
            names = names + "\"" + "dimension " + std::to_string(i) + "\"";

        if (i < numDimensions - 1) names = names + ",";
        else names = names + "]";
    }

    _jsonObject = "{\n" + nodes + names + "\n}";

    qDebug() << _jsonObject.c_str();

    emit _communicationObject->qt_setData(QString(_jsonObject.c_str()));
}

void HeatMapWidget::setSelection(QList<int> selection)
{
    emit _communicationObject->qt_setSelection(selection);
}

void HeatMapWidget::mousePressEvent(QMouseEvent *event)
{
    // UNUSED
}

void HeatMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    // UNUSED
}

void HeatMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // UNUSED
}

void HeatMapWidget::onSelection(QRectF selection)
{
    // UNUSED
}

void HeatMapWidget::cleanup()
{

}

void HeatMapWidget::initWebPage()
{
    loaded = true;

    for (QString option : dataOptionBuffer) {
        emit _communicationObject->qt_addAvailableData(option);
    }
    dataOptionBuffer.clear();
}

void HeatMapWidget::js_selectData(const QString& name)
{
    emit dataSetPicked(name);
}

void HeatMapWidget::js_selectionUpdated(const QVariantList& selectedClusters)
{
    std::vector<std::uint32_t> selectedIndices;
    for (std::uint32_t i = 0; i < selectedClusters.size(); ++i)
        if(selectedClusters[i].toInt() > 0)
            selectedIndices.push_back(i);

    emit clusterSelectionChanged(selectedIndices);
}
