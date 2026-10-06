#ifndef NAMELISTPARSER_H
#define NAMELISTPARSER_H

#include <QStringList>

class NameListParser
{
public:
    NameListParser();
    static QStringList parse(const QString& text);
};

#endif // NAMELISTPARSER_H
