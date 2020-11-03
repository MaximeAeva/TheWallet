#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->date_em->setDate(QDate::currentDate());
    this->setWindowTitle(Readconfig("WINDOW"));
    this->setWindowIcon(QIcon(Readconfig("ICON")));

    if(ConnectDB())
    {
        std::cout<<"you're connected"<<std::endl;
        loadhistoric();
        loadgraphic();
        connect(ui->save, SIGNAL(clicked()), this, SLOT(savequery()));
    }
    else {
        std::cout<<"Connection failed"<<std::endl;
        qDebug() << db.drivers();
        qDebug() << db.lastError();
    }
}

void MainWindow::savequery()
{
    double montant = ui->montant->value();
    QString context = ui->context->currentText();
    QDate date_em = ui->date_em->date();
    bool debit = ui->debit->checkState();
    bool autom = ui->auto_2->checkState();
    QString com = ui->com->text();

    QSqlQuery query;
    query.prepare("INSERT INTO transaction (montant, date_em, context, debit, auto, com)"
                  "VALUES (:montant, :date_em, :context, :debit, :auto, :com)");
    query.bindValue(":montant", montant);
    query.bindValue(":date_em", date_em);
    query.bindValue(":context", context);
    query.bindValue(":debit", debit);
    query.bindValue(":auto", autom);
    query.bindValue(":com", com);
    query.exec();

    ui->montant->clear();
    ui->debit->clearMask();
    ui->auto_2->clearMask();
    ui->com->clear();
    ui->date_em->setDate(QDate::currentDate());

    loadhistoric();
    loadgraphic();
}

void MainWindow::loadhistoric()
{
    QSqlQueryModel *mod = new QSqlQueryModel();
    QSqlQuery *query = new QSqlQuery(db);
    query->prepare("SELECT * FROM transaction ORDER BY date_em DESC");
    query->exec();
    mod->setQuery(*query);
    ui->tableView->setModel(mod);
}

void MainWindow::loadgraphic()
{
    //Percentage progress bar
    QSqlQueryModel *perc = new QSqlQueryModel;
    perc->setQuery("SELECT 100-(SELECT SUM(montant) FROM transaction WHERE debit='True' "
                "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                "FROM transaction WHERE context = 'Salaire' LIMIT 1))*100/"
                "(SELECT SUM(montant) FROM transaction WHERE debit='False' "
                "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                "FROM transaction WHERE context = 'Salaire' LIMIT 1)) as p");

    //Total spent since last salary
    QSqlQueryModel *total = new QSqlQueryModel;
    total->setQuery("SELECT SUM(montant) as tot FROM transaction WHERE debit='True' "
                    "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                    "FROM transaction WHERE context = 'Salaire' LIMIT 1)");

    //Context share this month
    QPieSeries *series = new QPieSeries();
    QSqlQueryModel *counter = new QSqlQueryModel;
    counter->setQuery("SELECT COUNT(DISTINCT context) as menu FROM transaction"
                      " WHERE context <> 'Salaire' AND date_em >= (SELECT LAST_VALUE(date_em) "
                      "OVER (ORDER BY date_em DESC) FROM transaction WHERE context = 'Salaire' LIMIT 1)");
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT SUM(montant) as montant, context FROM transaction"
                    " WHERE debit='True' AND date_em >= (SELECT LAST_VALUE(date_em) "
                    "OVER (ORDER BY date_em DESC) FROM transaction WHERE context = 'Salaire' LIMIT 1) GROUP BY context");
    for(int i=0; i<counter->record(0).value("menu").toInt();i++)
    {
        series->append(model->record(i).value("context").toString(),model->record(i).value("montant").toFloat());
        series->slices().at(i)->setLabelVisible();

    }

   QChart *chart = new QChart();
   chart->addSeries(series);
   chart->setTitle("Dépenses");
   chart->legend()->hide();

   //Owned money through the age
    QLineSeries *lines = new QLineSeries();
    QSqlQueryModel *timeserie = new QSqlQueryModel;
    timeserie->setQuery("SELECT date_em AS date, SUM(CASE WHEN debit='t' THEN -montant ELSE montant END) AS ts "
                        "FROM transaction GROUP BY date ORDER BY date");
    QSqlQueryModel *count = new QSqlQueryModel;
    count->setQuery("SELECT COUNT(DISTINCT date_em) as cnt FROM transaction");
    float totalMoney = 0;
    for(int i=0; i<count->record(0).value("cnt").toInt();i++)
    {
        totalMoney += timeserie->record(i).value("ts").toFloat();
        QDateTime d = timeserie->record(i).value("date").toDateTime();
        lines->append(d.toMSecsSinceEpoch(),totalMoney);
    }

    QChart *chart2 = new QChart();
    chart2->addSeries(lines);
    chart2->legend()->hide();
    chart2->setTitle("OverTime");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("dd MMM");
    axisX->setTitleText("Date");
    chart2->addAxis(axisX, Qt::AlignBottom);
    lines->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(10);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Value");
    chart2->addAxis(axisY, Qt::AlignLeft);
    lines->attachAxis(axisY);

    //Shared overall
    QPieSeries *overallPie = new QPieSeries();
    QSqlQueryModel *contextCnt = new QSqlQueryModel;
    contextCnt->setQuery("SELECT COUNT(DISTINCT context) as cnt FROM transaction");
    QSqlQueryModel *model2 = new QSqlQueryModel;
    model2->setQuery("SELECT context as ctx, SUM(montant) as value FROM transaction "
                     "WHERE debit = 't' GROUP BY context");
    for(int i=0; i<contextCnt->record(0).value("cnt").toInt();i++)
    {
        overallPie->append(model2->record(i).value("ctx").toString(),model2->record(i).value("value").toFloat());
        overallPie->slices().at(i)->setLabelVisible();

    }

    QChart *chart3 = new QChart();
    chart3->addSeries(overallPie);
    chart3->setTitle("Répartition générale");
    chart3->legend()->hide();

    //Bar chart mean spent per month
    QBarSet *set = new QBarSet("depense");
    QStringList month;
    QSqlQueryModel *allCnt = new QSqlQueryModel;
    allCnt->setQuery("SELECT COUNT(*) as cnt FROM transaction");
    QSqlQueryModel *model3 = new QSqlQueryModel;
    model3->setQuery("SELECT montant as mt, debit as deb, date_em as dat FROM transaction ORDER BY dat");
    float k = 0;
    float meanSpent = 0;
    int indexor = 0;
    for(int i=0; i<allCnt->record(0).value("cnt").toInt();i++)
    {
        if(!model3->record(i).value("deb").toBool())
        {
            indexor ++;
            QDateTime d = model3->record(i).value("dat").toDateTime();
            month << d.toString("MMM");
            *set << k;
            meanSpent += k;
            k = 0;
        }
        else k += model3->record(i).value("mt").toFloat();

    }

    QLineSeries *myMean = new QLineSeries;
    for(int i = 0; i<indexor; i++) myMean->append(i, meanSpent/indexor);

    QPen pen = myMean->pen();
    pen.setWidth(1);
    pen.setBrush(QBrush("red"));
    myMean->setPen(pen);

    QBarSeries *barseries = new QBarSeries();
    barseries->append(set);

    QChart *chart4 = new QChart();
    chart4->addSeries(barseries);
    chart4->addSeries(myMean);
    chart4->setTitle("Dépense par mois");
    chart4->legend()->hide();

    QBarCategoryAxis *axisX1 = new QBarCategoryAxis();
    axisX1->append(month);
    chart4->addAxis(axisX1, Qt::AlignBottom);
    barseries->attachAxis(axisX1);
    myMean->attachAxis(axisX1);

    QValueAxis *axisY1 = new QValueAxis();
    chart4->addAxis(axisY1, Qt::AlignLeft);
    barseries->attachAxis(axisY1);
    myMean->attachAxis(axisY1);
    axisY1->setTickCount(10);

   ui->graphicsView->setChart(chart);

   ui->graphicsView_2->setChart(chart2);

   ui->graphicsView_3->setChart(chart3);

   ui->graphicsView_4->setChart(chart4);

   ui->totalDebit->setText(total->record(0).value("tot").toString());

   ui->progressBar->setValue(perc->record(0).value("p").toInt());
}

MainWindow::~MainWindow()
{
    db.close();
    delete ui;
}

QString MainWindow::Readconfig(std::string paramName)//Read config file
{
    std::ifstream cfg("config.txt");
    if(!cfg.is_open())
    {
        std::cout << "failed" << std::endl;
        return 0;
    }
    std::string parm, value;
    while (cfg >> parm >> value)
    {
        if(parm== paramName){return QString::fromStdString(value);}
    }
    return 0;
}

bool MainWindow::ConnectDB()//Connect to DB
{
    db = QSqlDatabase::addDatabase(Readconfig("DRIVER"));
    db.setHostName(Readconfig("HOST"));
    db.setDatabaseName(Readconfig("NAME"));
    db.setUserName(Readconfig("USER"));
    db.setPassword(Readconfig("PASSWORD"));
    if(db.open())
    {
        return 1;
    }
    else
    {
        qDebug() << db.lastError();
        return 0;
    }
}

