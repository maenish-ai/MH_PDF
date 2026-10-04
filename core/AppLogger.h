#pragma once
#include <QString>
class AppLogger {
public:
    static void install();
    static QString logFilePath();
    static void write(const QString& level,const QString& message);
};
