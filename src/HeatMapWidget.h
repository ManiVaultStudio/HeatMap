#pragma once

#include "widgets/WebWidget.h"

#include <cstdint>

#include <QList>
#include <QMouseEvent>
#include <QVariant>

class QWebEngineView;
class QWebEnginePage;
class QWebChannel;

class Cluster;

class HeatMapWidget;

class HeatMapCommunicationObject : public mv::gui::WebCommunicationObject
{
    Q_OBJECT
public:
    HeatMapCommunicationObject(HeatMapWidget* parent);

signals:
    void qt_setData(QString data);
    void qt_addAvailableData(QString name);
    void qt_setSelection(QList<int> selection);
    void qt_setHighlight(int highlightId);
    void qt_setMarkerSelection(QList<int> selection);

public slots:
    void js_selectData(QString text);
    void js_selectionUpdated(const QVariantList& selectedClusters);

private:
    HeatMapWidget* _parent;
};

class HeatMapWidget : public mv::gui::WebWidget
{
    Q_OBJECT
public:
    HeatMapWidget();
    ~HeatMapWidget() override;

    void addDataOption(const QString option);
    void setData(const QVector<Cluster>& data, const std::vector<QString>& dimNames, const std::vector<QString>& clusterNames, const int numDimensions);
    void setSelection(QList<int> selection);

protected:
    void mousePressEvent(QMouseEvent *event)   Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event)    Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void onSelection(QRectF selection);
    void cleanup();

signals:
    void clusterSelectionChanged(const std::vector<std::uint32_t>& selectedClusters);
    void dataSetPicked(const QString& name);

public:
    void js_selectData(const QString& text);
    void js_selectionUpdated(const QVariantList& selectedClusters);

private slots:
    void initWebPage() override;

private:
    HeatMapCommunicationObject* _communicationObject;

    unsigned int _numClusters;

    /** Whether the web view has loaded and web-functions are ready to be called. */
    bool loaded;
    /** Temporary storage for added data options until webview is loaded */
    QList<QString> dataOptionBuffer;
};
