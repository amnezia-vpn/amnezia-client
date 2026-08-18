#ifndef MIGRATIONS_H
#define MIGRATIONS_H

#include <QObject>

class Migrations : public QObject
{
    Q_OBJECT
public:
    explicit Migrations(QObject *parent = nullptr);

    void doMigrations();

private:
    void migrateV3();

private:
    int currentMajor = 0;
    int currentMinor = 0;
    int currentMicro = 0;
    int currentPatch = 0;
};

#endif // MIGRATIONS_H
