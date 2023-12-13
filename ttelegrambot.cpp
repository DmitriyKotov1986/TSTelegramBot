#include <QtSql/QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QImage>
#include <QPainter>
#include "ttelegrambot.h"

TTelegramBot::TTelegramBot(QSettings& Config, QObject *parent)
  : QObject(parent)
  , Config(Config)
{
    Config.beginGroup("DATABASE");
    DB = QSqlDatabase::addDatabase(Config.value("Driver", "QODBC").toString(), "MainDB");
    DB.setDatabaseName(Config.value("DataBase", "SystemMonitorDB").toString());
    DB.setUserName(Config.value("UID", "SYSDBA").toString());
    DB.setPassword(Config.value("PWD", "MASTERKEY").toString());
    DB.setConnectOptions(Config.value("ConnectionOprions", "").toString());
    DB.setPort(Config.value("Port", "3051").toUInt());
    DB.setHostName(Config.value("Host", "localhost").toString());
    Config.endGroup();

    if (!DB.open()) {
        qDebug() << "Cannot connect to database. Error: " << DB.lastError().text();
        exit(-10);
    };

    Config.beginGroup("DATABASE");
    LogDB = QSqlDatabase::addDatabase(Config.value("Driver", "QODBC").toString(), "LogDB");
    LogDB.setDatabaseName(Config.value("DataBase", "SystemMonitorDB").toString());
    LogDB.setUserName(Config.value("UID", "SYSDBA").toString());
    LogDB.setPassword(Config.value("PWD", "MASTERKEY").toString());
    LogDB.setConnectOptions(Config.value("ConnectionOprions", "").toString());
    LogDB.setPort(Config.value("Port", "3051").toUInt());
    LogDB.setHostName(Config.value("Host", "localhost").toString());
    Config.endGroup();

    if (!LogDB.open()) {
        qDebug() << "Cannot connect to database. Error: " << DB.lastError().text();
        exit(-10);
    };

    Config.beginGroup("SYSTEM");
    Token = Config.value("Token", "").toString();
    if (Token == "") {
        qDebug() << "Token undefine";
        exit(-15);
    }
    TmpPath = Config.value("TmpPath", QCoreApplication::applicationDirPath()).toString();
    Config.endGroup();

    rg = QRandomGenerator::global();

   // SendLogMsg(MSG_CODE::CODE_OK, "Successfully started");
}

TTelegramBot::~TTelegramBot()
{
    SendLogMsg(MSG_CODE::CODE_OK, "Successfully finished");
    DB.close();
    LogDB.close();
}

void TTelegramBot::Start()
{
    bot = new Telegram::Bot(Token, true, 1000, 4, this);
    QObject::connect(bot, SIGNAL(message(Message)), this, SLOT(onNewMessage(Message)));
    QObject::connect(bot, SIGNAL(update(Update)), this, SLOT(onUpdate(Update)));

    qDebug() << "Start";
}

void TTelegramBot::Stop()
{ 
    bot->deleteLater();

    qDebug() << "Finished";
}

void TTelegramBot::onNewMessage(Telegram::Message message)
{
    qDebug() << "new message:" << message << message.string;

    if (message.type == Telegram::Message::TextType) {
        if(ChatMode[QString::number(message.from.id)].ChatMode == CHAT_MODE::CHAT_REQUEST_COUPON_STATUS) {
            QString ChatID = QString::number(message.from.id);
            SendCouponStatus(ChatID, message.string);
            ChatMode[QString::number(message.from.id)].ChatMode = CHAT_MODE::CHAT_OK;
            return;
        }
        else if (ChatMode[QString::number(message.from.id)].ChatMode == CHAT_MODE::CHAT_REQUEST_COUPON_STATUS_ON_AZS) {
            QString ChatID = QString::number(message.from.id);
         //   SendCouponStatusOnAZS(ChatID, ChatMode[QString::number(message.from.id)].Data, message.string);
            SendCouponStatusOnAllAZS(ChatID, message.string);
            ChatMode[QString::number(message.from.id)].ChatMode = CHAT_MODE::CHAT_OK;
            return;
        }

        ChatMode[QString::number(message.from.id)].ChatMode = CHAT_MODE::CHAT_OK;
        //подключаем бота
        if (message.string == "/start") {
            //добавляем кнопки
            KeyboardMarkup MenuButton;
            MenuButton.push_back(QStringList() << "Талоны" << "Файлы" << "Уровнемеры" << "ПК"/* << "Настройки"*/);
            ReplyKeyboardMarkup KeyboardMarkup(MenuButton, true, false, false);

            bot->sendMessage(message.from.id, "Бот SystemMonitorBot готов к использованию.\r\n"
                                              "Для получения информации по талонам - нажмите кнопку 'Талоны'.\r\n"
                                              "Для получения информации о файловом обмене - нажмите кнопку 'Файлы'.\r\n"
                                              "Для получения информации работе уровнемеров - нажмите кнопку 'Уровнемеры'.\r\n"
                                              "Для получения информации работе ПК - нажмите кнопку 'ПК'.\r\n"
                                             // "Для настройки параметров работы - нажмите кнопку 'Настройки'.\r\n"
                                              "Для отключения бота введите '/stop'", false, false, 0, KeyboardMarkup);
        }
        else if (message.string == "Талоны") {
            InlineKeyboardButton CouponsInfoKeyboardButton("Передача данных", "", "CI" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton CouponStatusKeyboardButton("Состояние талона", "", "CS" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton CouponStatusOnAZSKeyboardButton("Состояние талона на АЗС", "", "CA" + QString::number(message.from.id) , "", "");
            InlineKeyboardButtons KeyboardButtons;
            KeyboardButtons.push_back(CouponsInfoKeyboardButton);
            KeyboardButtons.push_back(CouponStatusKeyboardButton);
            KeyboardButtons.push_back(CouponStatusOnAZSKeyboardButton);
            InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);
            bot->sendMessage(message.from.id, "Выберите интересющие вас сведения", false, false, 0, KeyboardMarkup);
        }
        else if (message.string == "Файлы") {
            InlineKeyboardButton FileInfoKeyboardButton("Передача данных", "", "FI" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton FilesForAZSKeyboardButton("Последние загруженных 10 файлов", "", "FD" + QString::number(message.from.id) , "", "");
            InlineKeyboardButtons KeyboardButtons;
            KeyboardButtons.push_back(FileInfoKeyboardButton);
            KeyboardButtons.push_back(FilesForAZSKeyboardButton);
            InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);
            bot->sendMessage(message.from.id, "Выберите интересющие вас сведения", false, false, 0, KeyboardMarkup);
        }
        else if (message.string == "Уровнемеры") {
            InlineKeyboardButton LevelGaugeInfoKeyboardButton("Передача данных", "", "LI" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton LevelGaugeStatusKeyboardButton("Остатки топлива на АЗС", "", "LS" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton LevelGaugeStatusNITKeyboardButton("Остатки топлива на АЗС (НИТ)", "", "LN" + QString::number(message.from.id) , "", "");
 //           InlineKeyboardButton LevelGaugeIntakeKeyboardButton("Прием топлива на АЗС", "", "LN" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton LevelGaugeConfigKeyboardButton("Конфигурация резервуаров", "", "LC" + QString::number(message.from.id) , "", "");
            InlineKeyboardButtons KeyboardButtons; 
            KeyboardButtons.push_back(LevelGaugeInfoKeyboardButton);
            KeyboardButtons.push_back(LevelGaugeStatusKeyboardButton);
            KeyboardButtons.push_back(LevelGaugeStatusNITKeyboardButton);
//            KeyboardButtons.push_back(LevelGaugeIntakeKeyboardButton);
            KeyboardButtons.push_back(LevelGaugeConfigKeyboardButton);
            InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);
            bot->sendMessage(message.from.id, "Выберите интересющие вас сведения", false, false, 0, KeyboardMarkup);
        }
        else if (message.string == "ПК") {
            InlineKeyboardButton PCInfoKeyboardButton("Информация о ПК", "", "PI" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton ConnectKeyboardButton("Состояние связи", "", "PC" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton ResourseKeyboardButton("Ресурсы", "", "PR" + QString::number(message.from.id) , "", "");
            InlineKeyboardButtons KeyboardButtons;
            KeyboardButtons.push_back(PCInfoKeyboardButton);
            KeyboardButtons.push_back(ConnectKeyboardButton);
            KeyboardButtons.push_back(ResourseKeyboardButton);
            InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);
            bot->sendMessage(message.from.id, "Выберите интересющие вас сведения", false, false, 0, KeyboardMarkup);
        }
       /* else if (message.string == "Настройки") {
            InlineKeyboardButton SendNoteficationOnKeyboardButton("Включить", "", "SO" + QString::number(message.from.id) , "", "");
            InlineKeyboardButton SendNoteficationOffKeyboardButton("Отключить", "", "SF" + QString::number(message.from.id) , "", "");
            InlineKeyboardButtons KeyboardButtons;
            KeyboardButtons.push_back(SendNoteficationOnKeyboardButton);
            KeyboardButtons.push_back(SendNoteficationOffKeyboardButton);
            InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);
            bot->sendMessage(message.from.id, "Включить уведомления о сбоях", false, false, 0, KeyboardMarkup);
        }*/

        else if (message.string == "/stop") {
            //удаляем все старые кнопки
            ReplyKeyboardRemove DeleteReplyMarkup(false);
            bot->sendMessage(message.from.id, "Бот отключен", false, false, 0, DeleteReplyMarkup);
        }
        else {
            bot->sendMessage(message.from.id, "Неизместная команда: '" + message.string + "'");
        }
    }
    else {
        bot->sendMessage(message.from.id, "Присланное сообщение должно быть текстом");
    }
}

void TTelegramBot::onUpdate(Update update)
{
    qDebug() << "new update:" << update << " Data:" << update.callbackquery.data;
    bot->answerCallbackQuery(update.callbackquery.id, "Идет обработка запроса....", false, "", 0);

    QString CMD = update.callbackquery.data.left(2);
    QString ChatID = update.callbackquery.data.right(update.callbackquery.data.size() - 2);



    if (CMD == "CI") {
        SendCouponsInfo(ChatID);
    }
    else if (CMD == "CS") {
        bot->sendMessage(ChatID, "Введите код талона");
        ChatMode[ChatID].ChatMode = CHAT_MODE::CHAT_REQUEST_COUPON_STATUS;
    }
    // проверка кода талона на АЗС
    else if (CMD == "CA") {
      //  SelectAZSButton(ChatID, "CB");
        bot->sendMessage(ChatID, "Введите код талона");
        ChatMode[ChatID].ChatMode = CHAT_MODE::CHAT_REQUEST_COUPON_STATUS_ON_AZS;
    }
   /* else if (CMD == "CB") {
        QString Data = ChatID.left(ChatID.indexOf("A"));
        ChatID = ChatID.right(ChatID.length() - ChatID.indexOf("A") - 1);
        ChatMode[ChatID].ChatMode = CHAT_MODE::CHAT_REQUEST_COUPON_STATUS_ON_AZS;
        ChatMode[ChatID].Data = Data;
        bot->sendMessage(ChatID, "Введите код талона");
    }*/
    //состояние обмена файлами
    else if (CMD == "FI") {
        SendFilesInfo(ChatID);
    }
    //последние загруженные файлы
    else if (CMD == "FD") {
        SelectAZSButton(ChatID, "FA");
    }
    else if (CMD == "FA") {
        QString Data = ChatID.left(ChatID.indexOf("A")); //Data - код АЗС
        ChatID = ChatID.right(ChatID.length() - ChatID.indexOf("A") - 1);
        SelectFilesButton(ChatID, Data);
    }
    else if (CMD == "FC") {
        QString Data = ChatID.left(ChatID.indexOf("A"));//Data - код АЗС
        ChatID = ChatID.right(ChatID.length() - ChatID.indexOf("A") - 1);
        SendFile(ChatID, Data);
    }

     //уровнемеры
    else if (CMD == "LI") { //состояние получения данных от заправок
        SendLevelGaugeInfo(ChatID);
    }

    //остаток топлива на АЗС
    else if (CMD == "LS") {
        SelectAZSButton(ChatID, "LA");
    }
    else if (CMD == "LA") {
        QString Data = ChatID.left(ChatID.indexOf("A"));//Data - код АЗС
        ChatID = ChatID.right(ChatID.length() - ChatID.indexOf("A") - 1);
        SendStatusLevelGauge(ChatID, Data);
    }

    //Конфигурация резервуаров
    else if (CMD == "LC") {
        qDebug() << "agasgasg";
        SelectAZSButton(ChatID, "LB");
    }
    else if (CMD == "LB") {
        QString Data = ChatID.left(ChatID.indexOf("A"));//Data - код АЗС
        ChatID = ChatID.right(ChatID.length() - ChatID.indexOf("A") - 1);
        SendConfigLevelGauge(ChatID, Data);
    }

}

void TTelegramBot::SendLogMsg(uint16_t Category, const QString &Msg)
{
    QString Str(Msg);
    qDebug() << Str;

    QSqlQuery QueryLog(LogDB);
    LogDB.transaction();
    QString QueryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(Category) + ", "
                        "\'TSTelegramBot\', "
                        "\'" + Str +"\'"
                        ")";

    if (!QueryLog.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << QueryLog.lastError().text() << " Query: "<< QueryLog.lastQuery();
        LogDB.rollback();
        exit(-1);
    }
    if (!LogDB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << LogDB.lastError().text();
        LogDB.rollback();
        exit(-2);
    };
}

void AddMark(QImage& image)
{
    QImage mark(image.width(), image.height(),QImage::Format_RGB32);

    QPainter painter(&mark);
    painter.fillRect(0, 0, mark.width(), mark.height(), Qt::transparent);
    QFont ft = painter.font();
    ft.setPixelSize(20);
    painter.setPen(Qt::white);
    painter.setFont(ft);
    if (static_cast<double>(mark.width())/static_cast<double>(mark.height()) < 1.0)
    {
        painter.setTransform(QTransform()
            .translate(mark.width() / 2, mark.height() / 2)
            .rotate(0, Qt::XAxis)
            .rotate(0, Qt::YAxis)
            .rotate(45, Qt::ZAxis)
            .scale(1 , 1)
            .translate(-mark.width() / 2, -mark.height() / 2));
    }
    painter.drawText(0, 0, mark.width(), mark.height(), Qt::AlignCenter, "ТОО Управление автоматизации");
    painter.restore();

    const float alpha = 0.8;
    const float beta = 1 - alpha;

    for(int x = 0; x < mark.width(); x++)
    {
        for(int y = 0; y < mark.height(); y++)
        {
            const QRgb rgbMark = mark.pixel(x,y);
            const QRgb rgbSrc = image.pixel(x,y);

            int r = int(qRed(rgbSrc) * alpha + qRed(rgbMark) * beta);
            int g = int(qGreen(rgbSrc) * alpha + qGreen(rgbMark) * beta);
            int b = int(qBlue(rgbSrc) * alpha + qBlue(rgbMark) * beta);

            r = (0 <= r && r <= 255) ? r : 0;
            g = (0 <= g && g <= 255) ? g : 0;
            b = (0 <= b && b <= 255) ? b : 0;

            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
}

void TTelegramBot::SendCouponsInfo(const QString& ChatID)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT [AZSCode], [LastCouponsDateTime], (SELECT MAX(ID) FROM [Coupons]) - [LastRequestedCouponsID] AS LastID "
                        "FROM [CouponsInfo] "
                        "INNER JOIN (SELECT MAX([ID]) AS ID "
                                    "FROM [CouponsInfo] "
                                    "WHERE [AZSCode] IN (SELECT [AZSCode] FROM [AZSInfo] WHERE [AZSInfo].[IsAZS] = 1) "
                                    "GROUP BY [AZSCode] "
                        ") AS A "
                        "ON A.ID = [CouponsInfo].ID "
                        "ORDER BY [AZSCode]";

    qDebug() << "Query to DB";
    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        LogDB.rollback();
        exit(-1);
    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(440, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 499, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Код АЗС");
    Painter.drawText(90,16, "Дата/Время");
    Painter.drawText(300,16, "Не загружено");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    while (Query.next()) {
        qint64 SecToLastRequest = Query.value("LastCouponsDateTime").toDateTime().secsTo(QDateTime::currentDateTime());
        if (SecToLastRequest > 86400) {
            Painter.setBrush(Qt::red);
        }
        else if (SecToLastRequest > 3600) {
            Painter.setBrush(Qt::yellow);
        }
        else {
            if (Query.value("LastID").toULongLong() > 10000) {
                Painter.setBrush(Qt::blue);
            }
            else {
                Painter.setBrush(Qt::green);
            }
        }
        Painter.drawRect(0, CurrentY, 499, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, Query.value("AZSCode").toString());
        Painter.drawText(90, CurrentY + 16, Query.value("LastCouponsDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        Painter.drawText(300, CurrentY + 16, Query.value("LastID").toString());

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 440, CurrentY);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file: " << Image.save(FileName, "JPG", 70);

    QFile TmpFile(FileName);

    bot->sendPhoto(ChatID, &TmpFile, "Состояние обмена талонами на " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
}

void TTelegramBot::SendCouponStatus(const QString& ChatID, const QString& Code)
{
    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT TOP (1) [ID], [DateTime], [Status], [AZSCode] "
                        "FROM [Coupons] "
                        "WHERE [Code] = '" + Code + "'"
                        "ORDER BY [ID] DESC";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    if (Query.next()) {
        bot->sendMessage(ChatID, "Текущий статус: " + (Query.value("Status").toInt() == 0 ? QString("Погашен") : QString("Активен")) + "\r\n" +
                                 "Изменен: " + Query.value("DateTime").toDateTime().toString("yyyy-MM-dd hh:mm") + "\r\n" +
                                 "Источник: " + (Query.value("AZSCode").toString() == "000" ? QString("1С") : ("АЗС" + Query.value("AZSCode").toString())));
    }
    else {
        bot->sendMessage(ChatID, "Нет данных по талону '" + Code + "'");
    }

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    }
}

void TTelegramBot::SendCouponStatusOnAZS(const QString &ChatID, const QString &AZSCode, const QString &Code)
{
    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT TOP (1) [ID], [DateTime], [Code], [Status], [AZSCode] "
                        "FROM [Coupons] "
                        "WHERE [Code] ='" + Code + "' AND [ID] <= (SELECT MAX(LastRequestedCouponsID) "
                                                                "FROM [CouponsInfo] "
                                                                "WHERE [AZSCode] = '" + AZSCode + "') "
                        "ORDER BY [ID] DESC";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        LogDB.rollback();
        exit(-1);
    }

    if (Query.next()) {
        bot->sendMessage(ChatID, "АЗС: " + AZSCode + "\r\n" +
                                 "Текущий статус: " + (Query.value("Status").toInt() == 0 ? QString("Погашен") : QString("Активен")) + "\r\n" +
                                 "Изменен: " + Query.value("DateTime").toDateTime().toString("yyyy-MM-dd hh:mm") + "\r\n" +
                                 "Источник: " + (Query.value("AZSCode").toString() == "000" ? QString("1С") : ("АЗС" + Query.value("AZSCode").toString())));
    }
    else {
        bot->sendMessage(ChatID, "Нет данных по талону '" + Code + "'");
    }

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    }
}

void TTelegramBot::SendCouponStatusOnAllAZS(const QString &ChatID, const QString &Code)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT Coupons.[Status], CouponsInfo.[AZSCode], CouponsInfo.[LastCouponsDateTime] "
                        "FROM ( SELECT MAX( a.[AZSID]) as [AZSID], a.[AZSCode], MAX(a.[ID]) as [ID], a.[Code] "
                               "FROM (SELECT MIN( CouponsInfo.[ID]) as [AZSID] ,CouponsInfo.[AZSCode], Coupons.[ID] AS [ID], Coupons.[Code] "
                                     "FROM [Coupons] as Coupons "
                                     "INNER JOIN [CouponsInfo] as CouponsInfo "
                                     "ON (Coupons.[ID] <= CouponsInfo.LastRequestedCouponsID) AND (Coupons.[Code] = '" + Code + "') "
                                     "GROUP BY CouponsInfo.[AZSCode], Coupons.[ID], Coupons.[Code]) AS a "
                               "GROUP BY a.[AZSCode], a.[Code]) as a "
                               "INNER JOIN [Coupons] as Coupons "
                               "ON a.[ID] = Coupons.[ID] "
                        "INNER JOIN [CouponsInfo] as CouponsInfo "
                        "ON a.[AZSID] = CouponsInfo.[ID] "
                        "WHERE CouponsInfo.[AZSCode] IN (SELECT [AZSCode] FROM [AZSInfo] WHERE [IsAZS] = 1) "
                        "ORDER BY CouponsInfo.[AZSCode] ";

    qDebug() << "Query to DB";
    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        LogDB.rollback();
        exit(-1);
    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(440, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 499, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Код АЗС");
    Painter.drawText(90,16, "Статус");
    Painter.drawText(230,16, "Дата/Время");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    while (Query.next()) {
        Painter.setBrush(Query.value("Status").toInt() == 0 ? Qt::red : Qt::green);

        Painter.drawRect(0, CurrentY, 499, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, Query.value("AZSCode").toString());
        Painter.drawText(90, CurrentY + 16, (Query.value("Status").toInt() == 0 ? "Не действует" : "Действует"));
        Painter.drawText(230, CurrentY + 16, Query.value("LastCouponsDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 440, CurrentY);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file";
    Image.save(FileName, "JPG", 70);

    if (CurrentY == 20) {
        bot ->sendMessage(ChatID, "Нет данных по талону '" + Code + "'");
    }
    else {
        QFile TmpFile(FileName);
        bot->sendPhoto(ChatID, &TmpFile, "Состояние талона '" + Code + "' на " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
    }
}

void TTelegramBot::SendFilesInfo(const QString &ChatID)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    Query.setForwardOnly(true);
    DB.transaction();

    qDebug() << "Query to DB";
    //получаем список АЗС
    QMap<QString, QPair<QString, QString>> AZSList;

    QString QueryText = "SELECT A.[AZSCode], MAX(DateTime) As LastRequestDateTime "
                        "FROM [SyncFileToClientInfo] AS A "
                        "INNER JOIN (SELECT [AZSCode] FROM [AZSInfo] WHERE [IsAZS] = 1) AS B "
                        "ON A.[AZSCode] = B.[AZSCode] "
                        "GROUP BY A.[AZSCode]" ;

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    while (Query.next()) {
        AZSList.insert(Query.value("AZSCode").toString(), qMakePair("0", Query.value("LastRequestDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    }

    //получаем недозагруженные файлы
    QueryText = "SELECT COUNT(A.[ID]) AS CountFiles, A.[AZSCode] "
                "FROM [SyncFileInfo] AS A "
                "INNER JOIN (SELECT MAX([DateTime]) AS DateTime, [AZSCode] ,MAX([LastRequestFileID]) AS LastID "
                            "FROM [SyncFileToClientInfo] "
                            "WHERE [AZSCode] in (SELECT [AZSCode] FROM [AZSInfo] WHERE [IsAZS] = 1) "
                            "GROUP BY [AZSCode]) AS B "
                "ON A.[AZSCode] = B.[AZSCode] "
                "WHERE  A.[ID] > B.[LastID] "
                "GROUP BY A.[AZSCode] "
                "ORDER BY  A.[AZSCode]";


    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    while (Query.next()) {
        AZSList[Query.value("AZSCode").toString()] = qMakePair(Query.value("CountFiles").toString(), AZSList[Query.value("AZSCode").toString()].second);

    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(440, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 499, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Код АЗС");
    Painter.drawText(90,16, "Дата/Время");
    Painter.drawText(300,16, "Не загружено");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    for (const auto& AZSCodeItem : AZSList.keys()) {
        //first - количество файлов, second - время последнего обмена

        qint64 SecToLastRequest = QDateTime::fromString(AZSList[AZSCodeItem].second, "yyyy-MM-dd hh:mm:ss").secsTo(QDateTime::currentDateTime());
        if (SecToLastRequest > 86400) {
            Painter.setBrush(Qt::red);
        }
        else if (SecToLastRequest > 3600) {
            Painter.setBrush(Qt::yellow);
        }
        else {
            if (AZSList[AZSCodeItem].first.toULongLong() > 0) {
                Painter.setBrush(Qt::blue);
            }
            else {
                Painter.setBrush(Qt::green);
            }
        }
        Painter.drawRect(0, CurrentY, 499, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, AZSCodeItem);
        Painter.drawText(90, CurrentY + 16, AZSList[AZSCodeItem].second);
        Painter.drawText(300, CurrentY + 16, AZSList[AZSCodeItem].first);

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 440, CurrentY);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file";
    Image.save(FileName, "JPG", 70);

    QFile TmpFile(FileName);

    bot->sendPhoto(ChatID, &TmpFile, "Состояние обмена файлами на " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
}

void TTelegramBot::SendFile(const QString &ChatID, const QString &FileID)
{
    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT [Body], [FileName] "
                        "FROM [SyncFileToClient] "
                        "WHERE [ID] = " + FileID + " ";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        LogDB.rollback();
        exit(-1);
    }

    if (Query.next()) {
        //декодируем тело файла
        QByteArray DecodeBody;
        auto result = QByteArray::fromBase64Encoding(Query.value("Body").toByteArray());
        if (result.decodingStatus == QByteArray::Base64DecodingStatus::Ok) {
            DecodeBody = result.decoded; // Декодируем файл
        }
        else {
            qDebug() << "Cannot decode file from Base64. ID:" << FileID << "FileName:" << Query.value("FileName").toString();
            return ;
        }
        //создаем директорию, если ее нет
        QString FullFileName = TmpPath + "/" + Query.value("FileName").toString();
        QDir Dir;
        Dir.mkpath(QFileInfo(FullFileName).absolutePath());
        //пишем файл
        QFile File(FullFileName, this);
        if (File.open(QIODeviceBase::WriteOnly)) {
            File.write(DecodeBody);
            File.close();
            qDebug() << "The file was saved successfully. File name:" << FullFileName <<
                        "Size:" << QString::number(DecodeBody.size());
        }
        else {
            qDebug() <<  "Cannot write file. File name:" << FullFileName;
            return;
        }

        bot->sendDocument(ChatID, &File);
    }


    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    }
}

void TTelegramBot::SendLevelGaugeInfo(const QString &ChatID)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    Query.setForwardOnly(true);
    DB.transaction();

    qDebug() << "Query to DB";
    QString QueryText = "SELECT b.[DateTime], b.[AZSCode], a.[TimeShift] "
                        "FROM [AZSInfo] AS a "
                        "INNER JOIN (SELECT MAX([DateTime]) AS [DateTime], [AZSCode] "
                                    "FROM [TanksMeasument] "
                                    "GROUP BY [AZSCode]) AS b "
                        "ON a.[AZSCode] = b.AZSCode "
                        "WHERE a.[AZSCode] IN (SELECT [AZSCode] FROM [AZSInfo] WHERE [IsAZS] = 1) "
                        "ORDER BY [AZSCode]";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(290, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 290, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Код АЗС");
    Painter.drawText(90,16, "Дата/Время");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    while (Query.next()) {

        qint64 SecToLastRequest = Query.value("DateTime").toDateTime().secsTo(QDateTime::currentDateTime()) + Query.value("TimeShift").toInt();
        if (SecToLastRequest > 3600) {
            Painter.setBrush(Qt::red);
        }
        else if (SecToLastRequest > 600) {
            Painter.setBrush(Qt::yellow);
        }
        else {
            Painter.setBrush(Qt::green);
        }
        Painter.drawRect(0, CurrentY, 499, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, Query.value("AZSCode").toString());
        Painter.drawText(90, CurrentY + 16, Query.value("DateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 290, CurrentY);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file";
    Image.save(FileName, "JPG", 70);

    QFile TmpFile(FileName);

    bot->sendPhoto(ChatID, &TmpFile, "Состояние обмена данными от уровнемеров на " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
}

void TTelegramBot::SendStatusLevelGauge(const QString &ChatID, const QString &AZSCode)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    Query.setForwardOnly(true);
    DB.transaction();

    qDebug() << "Query to DB";
    QString QueryText = "SELECT [TimeShift] "
                        "FROM [AZSInfo] "
                        "WHERE [AZSCode] =  '" + AZSCode + "'";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    qint64 TimeShift = 0;
    if (Query.next()) {
        TimeShift = Query.value("TimeShift").toLongLong();
    }

    QueryText = "SELECT d.[Product], d.[TankNumber], c.[Volume], [Mass], [Density], [Height], [Water], [Temp], c.[DateTime], d.[LastSaveDateTime], d.[LastIntakeDateTime]  "
                "FROM [TanksInfo] AS d "
                "INNER JOIN (SELECT  a.[DateTime], b.[TankNumber], b.[Volume], [Mass], [Density], [Height], [Water], [Temp] "
                            "FROM [TanksMeasument] AS b "
                            "INNER JOIN (SELECT MAX([DateTime]) AS [DateTime], [TankNumber] "
                                        "FROM [TanksMeasument] "
                                        "WHERE [AZSCode] = '" + AZSCode + "' AND [TankNumber] IN (SELECT [TankNumber] "
                                                                                                 "FROM [TanksInfo] "
                                                                                                 "WHERE [AZSCode] = '" + AZSCode + "' AND [Enabled] = 1) "
                                        "GROUP BY  [TankNumber]) AS a "
                            "ON (a.DateTime = b.DateTime AND a.TankNumber = b.TankNumber)) AS c "

                "ON c.[TankNumber] = d.[TankNumber] "
                "WHERE [AZSCode] = '" + AZSCode + "' "
                "ORDER by  d.[TankNumber]";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(1300, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 1300, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Емкость");
    Painter.drawText(100,16, "Объем(л)");
    Painter.drawText(200,16, "Уровень(мм)");
    Painter.drawText(320,16, "Пл-ть(кг/м3)");
    Painter.drawText(440,16, "Масса(кг)");
    Painter.drawText(540,16, "Tемп(гр)");
    Painter.drawText(640,16, "Вода(мм)");
    Painter.drawText(740,16, "АЗС(Дата/Время)");
    Painter.drawText(920,16, "НИТ(Дата/Время)");
    Painter.drawText(1100,16, "Приход(Дата/Время)");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    while (Query.next()) {

        qint64 SecToLastRequest = Query.value("DateTime").toDateTime().addSecs(-TimeShift).secsTo(QDateTime::currentDateTime());
        if (SecToLastRequest > 3600) {
            Painter.setBrush(Qt::red);
        }
        else if (SecToLastRequest > 600) {
            Painter.setBrush(Qt::yellow);
        }
        else {
            Painter.setBrush(Qt::green);
        }
        Painter.drawRect(0, CurrentY, 1300, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, Query.value("TankNumber").toString() + " (" + Query.value("Product").toString() + ")");
        Painter.drawText(100, CurrentY + 16, Query.value("Volume").toString());
        Painter.drawText(200, CurrentY + 16, Query.value("Height").toString());
        Painter.drawText(320, CurrentY + 16, Query.value("Density").toString());
        Painter.drawText(440, CurrentY + 16, Query.value("Mass").toString());
        Painter.drawText(540, CurrentY + 16, Query.value("Temp").toString());
        Painter.drawText(640, CurrentY + 16, Query.value("Water").toString());
        Painter.drawText(740, CurrentY + 16, Query.value("DateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        Painter.drawText(920, CurrentY + 16, Query.value("LastSaveDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        Painter.drawText(1100, CurrentY + 16, Query.value("LastIntakeDateTime").toDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 1300, std::max(CurrentY, 1300/20));

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file";
    Image.save(FileName, "JPG", 70);

    QFile TmpFile(FileName);

    bot->sendPhoto(ChatID, &TmpFile, "Текущие показания уровнемера на АЗС " + AZSCode + " Время: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
}

void TTelegramBot::SendConfigLevelGauge(const QString &ChatID, const QString &AZSCode)
{
    QString FileName = TmpPath + "/" + QString::number(rg->generate64()) + ".jpg"; //генерируем имя файла

    QSqlQuery Query(DB);
    Query.setForwardOnly(true);
    DB.transaction();

    QString QueryText = "SELECT [TankNumber], [Enabled], [Volume], [Diametr], [Tilt], [TCCoef], [Offset], [Product], [Description] "
                        "FROM [16].[dbo].[TanksInfo] "
                        "WHERE [AZSCode] = '" + AZSCode + "' "
                        "ORDER by  d.[TankNumber]";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    qDebug() << "Create file:" << FileName;
    //создаем пустую картинку
    QImage Image(1300, 5000, QImage::Format_RGB32);
    QPainter Painter(&Image);
    //делаем заготовок
    QFont Font(Painter.font());
    Font.setPixelSize(16);
    Font.setBold(true);
    Painter.setFont(Font);
    Painter.setBrush(Qt::white);
    Painter.drawRect(0, 0, 1300, 20);
    //Формируем шапку таблицы
    Painter.setPen(Qt::black);
    Painter.drawText(2,16, "Емкость");
    Painter.drawText(100,16, "Вкл");
    Painter.drawText(200,16, "Продукт");
    Painter.drawText(320,16, "Объем(л)");
    Painter.drawText(440,16, "Диам(мм)");
    Painter.drawText(540,16, "Наклон(гр)");
    Painter.drawText(640,16, "ТКоэф");
    Painter.drawText(740,16, "Смещ(мм)");
    Painter.drawText(920,16, "Примечания");

    Font.setBold(false);
    Painter.setFont(Font);

    int CurrentY = 20;
    while (Query.next()) {
        if (Query.value("Enabled").toInt() == 0) {
            Painter.setBrush(Qt::yellow);
        }
        else {
            Painter.setBrush(Qt::green);
        }
        Painter.drawRect(0, CurrentY, 1300, CurrentY + 20);

        Painter.drawText(2, CurrentY + 16, Query.value("TankNumber").toString() + " (" + Query.value("Product").toString() + ")");
        Painter.drawText(100, CurrentY + 16, Query.value("Enabled").toString());
        Painter.drawText(200, CurrentY + 16, Query.value("Product").toString());
        Painter.drawText(320, CurrentY + 16, Query.value("Volume").toString());
        Painter.drawText(440, CurrentY + 16, Query.value("Diametr").toString());
        Painter.drawText(540, CurrentY + 16, Query.value("Tilt").toString());
        Painter.drawText(640, CurrentY + 16, Query.value("TCCoef").toString());
        Painter.drawText(740, CurrentY + 16, Query.value("Offset").toString());
        Painter.drawText(920, CurrentY + 16, Query.value("Description").toString());

        CurrentY += 20;
    }

    Painter.end();

    //обрезаем изображение
    Image = Image.copy(0, 0, 1300, std::max(CurrentY, 1300/20));

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };

    qDebug() << "Add mark";
    AddMark(Image);

    qDebug() << "Save file";
    Image.save(FileName, "JPG", 70);

    QFile TmpFile(FileName);

    bot->sendPhoto(ChatID, &TmpFile, "Текущие показания уровнемера на АЗС " + AZSCode + " Время: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), 0, false);
}


void TTelegramBot::SelectAZSButton(const QString &ChatID, const QString& AddonsStr)
{
    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT [AZSCode] "
                        "FROM [AZSInfo] "
                        "WHERE [IsAzs] = 1"
                        "ORDER BY [AZSCode]";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        LogDB.rollback();
        exit(-1);
    }

    InlineKeyboardButtons KeyboardButtons;
    while (Query.next()) {
         InlineKeyboardButton AZSButton(Query.value("AZSCode").toString(), "", AddonsStr + Query.value("AZSCode").toString() + "A" + ChatID , "", "");
         KeyboardButtons.push_back(AZSButton);
    }
    InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons, 7);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    }

    bot->sendMessage(ChatID, "Выберите АЗС", false, false, 0, KeyboardMarkup);
}

void TTelegramBot::SelectFilesButton(const QString &ChatID, const QString& AZSCode)
{
    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT A.[ID], [FileName], [IsLoad] "
                        "FROM [SyncFileToClient] AS A "
                        "INNER JOIN (SELECT TOP (10) [ID], [HASH], IIF([LoadToAZSDateTime] IS NULL, 0, 1) AS IsLoad "
                                    "FROM [SyncFileInfo] "
                                    "WHERE [AZSCode] = '" + AZSCode + "' "
                                    "ORDER BY ID DESC) AS B "
                        "ON A.[HASH] = B.[HASH]";

    if (!Query.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << Query.lastError().text() << " Query: "<< Query.lastQuery();
        DB.rollback();
        exit(-1);
    }

    InlineKeyboardButtons KeyboardButtons;
    while (Query.next()) {
         InlineKeyboardButton AZSButton(Query.value("FileName").toString() +
                                        " (" + (Query.value("IsLoad").toInt() == 0 ? "Не доставлен" : "Доставлен" ) + ")",
                                        "",
                                        "FC" + Query.value("ID").toString() + "A" + ChatID , "", "");
         KeyboardButtons.push_back(AZSButton);
    }
    InlineKeyboardMarkup KeyboardMarkup(KeyboardButtons);

    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    }

    bot->sendMessage(ChatID, "Выберите файл (АЗС" + AZSCode + ")", false, false, 0, KeyboardMarkup);
}
