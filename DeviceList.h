//
// Created by Kamil Rojewski on 15.07.2021.
//

#ifndef OPENRGB_AMBIENT_DEVICELIST_H
#define OPENRGB_AMBIENT_DEVICELIST_H

#include <QWidget>

class ResourceManagerInterface;
class QTreeWidgetItem;
class QTreeWidget;
class Settings;

class DeviceList
        : public QWidget
{
    Q_OBJECT

public:
    DeviceList(ResourceManagerInterface *resourceManager, Settings &settings, QWidget *parent = nullptr);
    ~DeviceList() override = default;

signals:
    void controllerSelected(const QString &location);

public slots:
    void fillControllerList() const;

private slots:
    void onItemChanged(QTreeWidgetItem *item, int column) const;

private:
    ResourceManagerInterface *resourceManager;
    Settings &settings;
    QTreeWidget *deviceList;
};

#endif //OPENRGB_AMBIENT_DEVICELIST_H
