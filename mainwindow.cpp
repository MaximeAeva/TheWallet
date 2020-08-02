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
    QPieSeries *series = new QPieSeries();

        QSqlQueryModel *perc = new QSqlQueryModel;
        perc->setQuery("SELECT 100-(SELECT SUM(montant) FROM transaction WHERE debit='True' "
                    "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                    "FROM transaction WHERE context = 'Salaire' LIMIT 1))*100/"
                    "(SELECT SUM(montant) FROM transaction WHERE debit='False' "
                    "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                    "FROM transaction WHERE context = 'Salaire' LIMIT 1)) as p");

        QSqlQueryModel *total = new QSqlQueryModel;
        total->setQuery("SELECT SUM(montant) as tot FROM transaction WHERE debit='True' "
                        "AND date_em >= (SELECT LAST_VALUE(date_em) OVER (ORDER BY date_em DESC) "
                        "FROM transaction WHERE context = 'Salaire' LIMIT 1)");

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

           ui->graphicsView->setChart(chart);

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

