#include "sqlerrormapper.h"


namespace {

    // https://www.sqlite.org/rescode.html
    constexpr int kSqliteError = 1;
    constexpr int kSqliteBusy = 5;
    constexpr int kSqliteReadOnly = 8;
    constexpr int kSqliteFull = 13;
    constexpr int kSqliteCantOpen = 14;
    constexpr int kSqliteConstraint = 19;
    constexpr int kSqliteAuth = 23;

    // More code:
    // 19 | (N << 8), N - type
    constexpr int kSqliteConstraintCheck = 19 | (1 << 8);  // 275
    constexpr int kSqliteConstraintForeignKey = 19 | (3 << 8);  // 787
    constexpr int kSqliteConstraintNotNull = 19 | (5 << 8);  // 1299
    constexpr int kSqliteConstraintPrimaryKey = 19 | (6 << 8);  // 1555
    constexpr int kSqliteConstraintUnique = 19 | (8 << 8);  // 2067
}

ErrorCode mapSqlErrorToCode(const QSqlError& error) {
    const int native = error.nativeErrorCode().toInt();

    switch (native) {

    // --- Connection /  ---
    case kSqliteCantOpen:
    case kSqliteBusy:
        return ErrorCode::DatabaseConnectionLost;

    case kSqliteReadOnly:
        return ErrorCode::DatabaseReadOnly;

    case kSqliteAuth:
        return ErrorCode::DatabaseAccessDenied;

    case kSqliteFull:
        return ErrorCode::DatabaseDiskFull;

    // ---  ---
    case kSqliteConstraintUnique:
    case kSqliteConstraintPrimaryKey:
        return ErrorCode::DatabaseUniqueViolated;

    case kSqliteConstraintForeignKey:
        return ErrorCode::DatabaseForeignKeyViolated;

    case kSqliteConstraintNotNull:
    case kSqliteConstraintCheck:
        return ErrorCode::DatabaseConstraintViolated;

    case kSqliteConstraint:
        //
        return ErrorCode::DatabaseConstraintViolated;

    // --- Other ---
    case kSqliteError:
    default:
        return ErrorCode::DatabaseQueryFailed;
    }

}
