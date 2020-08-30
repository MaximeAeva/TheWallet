#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QGraphicsItem>
#include <QtCharts>
#include <QDate>
#include <vector>
#include <iostream>
#include <fstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void savequery();

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    void loadhistoric();
    void loadgraphic();
    QString Readconfig(std::string paramName);
    bool ConnectDB();
};
#endif // MAINWINDOW_H
