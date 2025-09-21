#include <QCoreApplication>
#include "DeviceClient.h"

int main(int argc, char **argv) {
    QCoreApplication a(argc, argv);
    DeviceClient client("127.0.0.1", 12345, &a);
    return a.exec();
}
