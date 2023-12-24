#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardItemModel>
#include <QTranslator>
#include <QList>
#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QDateTime>
#include "ParametersConnectionModbus.h"

//Create object Modbus RTU Answer
QList<int> *modbusRegisterAnswer = new QList<int>;
QList<unsigned int> *parseModbusAnswer = new QList<unsigned int>;
QComboBox *cmbxBaudrate;
int indexValue = -1;

void MainWindow::UpdateListCOMPorts(){
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    ui->cmbx_listSerialPorts->clear();

    for(const QSerialPortInfo &portInfo : serialPortInfos){
        serialPortParametersList.append(portInfo.portName());
        ui->cmbx_listSerialPorts->addItem(portInfo.portName());
        ui->txtbrw_logBrowser->append("Found Serial Posrt`s: " + portInfo.portName());
    }
}

void MainWindow::ConnectedModbusDevice(){
    SetupModbusParameters();
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, 0, 9);

    if (!modbusMaster->connectDevice()){
        ui->txtbrw_logBrowser->append("Error connected! - Device not found or not connected.");
        modbusMaster->disconnectDevice();
        return;
    }
    if (auto *reply = modbusMaster->sendReadRequest(readUnit, slaveAddressBits)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [=](){
                if (!reply)
                    return;
                if (reply->error() == QModbusDevice::NoError) {
                    const QModbusDataUnit data = reply->result();
                    modbusRegisterAnswer->clear();
                    for (int i = 0; i < data.valueCount(); i++) {
                        modbusRegisterAnswer->append(data.value(i));
                    }
                    ParseModbusResponse();
                }
                else if (reply->error() == QModbusDevice::ProtocolError) {
                    ui->txtbrw_logBrowser->append("Modbus protocol error:" + reply->errorString());
                }
                else {
                    ui->txtbrw_logBrowser->append("Modbus reply error:" + reply->errorString());
                }
                reply->deleteLater();
                modbusMaster->disconnectDevice();
            });
        }
        else{
            delete reply; //If response has already been completed, delete the response object.
        }
    }
    else {
        ui->txtbrw_logBrowser->append("Failed to send Modbus request: " + modbusMaster->errorString());
        modbusMaster->disconnectDevice();
    }
}

void MainWindow::ParseModbusResponse(){
    parseModbusAnswer->clear();
    parseModbusAnswer->resize(5);
    parseModbusAnswer->fill(0);

    parseModbusAnswer->replace(0, modbusRegisterAnswer->at(0));

    unsigned int baudrate = modbusRegisterAnswer->at(1);
    baudrate = (baudrate << 16) |  modbusRegisterAnswer->at(2);
    parseModbusAnswer->replace(1, baudrate);
    //ui->txtbrw_logBrowser->append(QString::number(parseModbusAnswer->at(1)));

    unsigned int totalizeOne = modbusRegisterAnswer->at(3);
    totalizeOne = (totalizeOne << 16) |  modbusRegisterAnswer->at(4);
    parseModbusAnswer->replace(2, totalizeOne);
    //ui->txtbrw_logBrowser->append(QString::number(modbusRegisterAnswer->at(2)));

    unsigned int totalizeTwo = modbusRegisterAnswer->at(5);
    totalizeTwo = (totalizeTwo << 16) |  modbusRegisterAnswer->at(6);
    parseModbusAnswer->replace(3, totalizeTwo);
    //ui->txtbrw_logBrowser->append(QString::number(modbusRegisterAnswer->at(3)));

    unsigned int totalizeThree = modbusRegisterAnswer->at(7);
    totalizeThree = (totalizeThree << 16) |  modbusRegisterAnswer->at(8);
    parseModbusAnswer->replace(4, totalizeThree);
    //ui->txtbrw_logBrowser->append(QString::number(modbusRegisterAnswer->at(4)));
    ResponseModbusDevice();
}

void MainWindow::ResponseModbusDevice(){
    QDateTime currentTime = QDateTime::currentDateTime();
    QString formattedTime = currentTime.toString("yyyy-MM-dd hh:mm:ss");

    auto *tableOutputDataParse = ui->tableView;
    QAbstractItemModel *qaim = tableOutputDataParse->model();
    cmbxBaudrate = new QComboBox;
    emit qaim->layoutAboutToBeChanged();

    for (int row = 0; row < qaim->rowCount(); row++) {
        QModelIndex index = qaim->index(row, 1);
        qaim->setData(index, parseModbusAnswer->at(row), Qt::EditRole);
        //ui->txtbrw_logBrowser->append("Success!");
    }

    emit qaim->layoutChanged();
    tableOutputDataParse->reset();

    for (const auto& baudrate : parametersListBaudrate)
    {
        cmbxBaudrate->addItem(QString::number(baudrate));
    }

    for (int i = 0; i < parametersListBaudrate.count(); i++){
        if(parametersListBaudrate.at(i) == parseModbusAnswer->at(1)){
            indexValue = i;
            break;
        }
    }
    cmbxBaudrate->setCurrentIndex(indexValue);
    tableOutputDataParse->setIndexWidget(tableOutputDataParse->model()->index(1, 1), cmbxBaudrate);
    ui->txtbrw_logBrowser->append("Modbus answer read success! Time read: " + formattedTime);
}

class ReadOnlyDelegate: public QStyledItemDelegate{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const override {
        return nullptr;
    }
};

void MainWindow::ParseModbusAnswer(){
    /*
    QStandardItemModel *model = new QStandardItemModel(0, 1, this); //create model with 0 rows, 1 column and use this class
    QTreeView *treeView = ui->treeView; //selected UI-object
    QStandardItem *rootItem = model->invisibleRootItem(); //retrun invisible root element (I don`t know nahuya)
    treeView->setRootIsDecorated(true); //show "arrow" for elements
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers); //ban on editable elements (Nehui rename something)
    treeView->setModel(model); //indicate use this model

    model->setHeaderData(0, Qt::Horizontal, "Parameters"); //Header text

    QStandardItem *item1 = new QStandardItem(QObject::tr("File"));
    rootItem->appendRow(item1);
    QStandardItem *childItem = new QStandardItem("Children elements 1");
    item1->appendRow(childItem);*/

    QStandardItemModel* model = new QStandardItemModel(0, 2, this);
    QTableView* tableView = ui->tableView;
    tableView->setModel(model);

    //add item`s to model
    QStandardItem* item1 = new QStandardItem("Modbus slave address");
    QStandardItem* item2 = new QStandardItem("Baudrate");
    QStandardItem* item3 = new QStandardItem("Totalize 1 (GENERAL)");
    QStandardItem* item4 = new QStandardItem("Totalize 2 (FORWARD)");
    QStandardItem* item5 = new QStandardItem("Totalize 3 (REVERSE)");
    model->appendRow(item1);
    model->appendRow(item2);
    model->appendRow(item3);
    model->appendRow(item4);
    model->appendRow(item5);

    QStandardItem* itemToUpdate = model->item(0);
    tableView->setItemDelegateForColumn(0, new ReadOnlyDelegate);

    model->itemChanged(itemToUpdate);
    tableView->update();
}

void MainWindow::WriteModbusDevice(){
    SetupModbusParameters();

    auto *tableOutputDataParse = ui->tableView;
    QAbstractItemModel *qaim = tableOutputDataParse->model();
    QModelIndex indexSlaveCell = qaim->index(0, 1);
    QVariant dataSlaveCell = qaim->data(indexSlaveCell);
    int slaveAddress = dataSlaveCell.toInt();

    int baudrateInteger = parametersListBaudrate.at(cmbxBaudrate->currentIndex());

    //ui->txtbrw_logBrowser->append(QString::number(Baudrate));

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 0, 3);

    int baudratePart_1 = (baudrateInteger >> 16) & 0xFFFF;
    int baudratePart_2 = baudrateInteger & 0xFFFF;

    writeUnit.setValue(0, slaveAddress);
    writeUnit.setValue(1, baudratePart_1);
    writeUnit.setValue(2, baudratePart_2);

    if (!modbusMaster->connectDevice()){
        ui->txtbrw_logBrowser->append("Error connected! - Device not found or not connected.");
        modbusMaster->disconnectDevice();
        return;
    }
    if (auto *write = modbusMaster->sendWriteRequest(writeUnit, slaveAddressBits)) {
        if (!write->isFinished()) {
            connect(write, &QModbusReply::finished, [=](){
                if (!write)
                    return;
                if (write->error() == QModbusDevice::NoError) {
                    ui->txtbrw_logBrowser->append("Success write!");
                }
                else if (write->error() == QModbusDevice::ProtocolError) {
                    ui->txtbrw_logBrowser->append("Modbus protocol error:" + write->errorString());
                }
                else {
                    ui->txtbrw_logBrowser->append("Modbus reply error:" + write->errorString());
                }
                write->deleteLater();
                modbusMaster->disconnectDevice();
            });
        }
        else{
            delete write;
        }
    }
    else {
        ui->txtbrw_logBrowser->append("Failed to send Modbus request: " + modbusMaster->errorString());
        modbusMaster->disconnectDevice();
    }
}