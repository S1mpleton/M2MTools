#ifndef SQLERRORMAPPER_H
#define SQLERRORMAPPER_H

#include "core/errorcode.h"

#include <QSqlError>

ErrorCode mapSqlErrorToCode(const QSqlError& error);

#endif // SQLERRORMAPPER_H
