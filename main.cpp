#include <QCoreApplication>
#include "service.h"

//для запуска ка консольное приложение запускать с параметром -e
int main(int argc, char *argv[])
{
    Service service(argc, argv);

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    return service.exec();
}
