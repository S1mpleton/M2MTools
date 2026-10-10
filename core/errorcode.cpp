#include "errorcode.h"

QString errorCodeToString(ErrorCode code) {
    switch (code) {
    // None
    case ErrorCode::None: return QStringLiteral("None");

    // Validation
    case ErrorCode::ADEmptyStateList: return QStringLiteral("EmptyStateList");
    case ErrorCode::ADEmptyInputList: return QStringLiteral("EmptyInputList");
    case ErrorCode::ADEmptyOutputList: return QStringLiteral("EmptyOutputList");
    case ErrorCode::ADTransitionTableSizeMismatch: return QStringLiteral("TransitionTableSizeMismatch");
    case ErrorCode::ADUnusedState: return QStringLiteral("UnusedState");
    case ErrorCode::ADUnusedInput: return QStringLiteral("UnusedInput");
    case ErrorCode::ADUnusedOutput: return QStringLiteral("UnusedOutput");

    case ErrorCode::ADInvalidName: return QStringLiteral("InvalidName");
    case ErrorCode::ADEmptyName: return QStringLiteral("EmptyName");
    case ErrorCode::ADInitialStateNotSet: return QStringLiteral("InitialStateNotSet");
    case ErrorCode::ADInitialStateNotFound: return QStringLiteral("InitialStateNotFound");

    // Database
    case ErrorCode::DatabaseOpenFailed: return QStringLiteral("DatabaseOpenFailed");
    case ErrorCode::DatabaseQueryFailed: return QStringLiteral("DatabaseQueryFailed");
    case ErrorCode::DatabaseInsertFailed: return QStringLiteral("DatabaseInsertFailed");
    case ErrorCode::DatabaseUpdateFailed: return QStringLiteral("DatabaseUpdateFailed");
    case ErrorCode::DatabaseDeleteFailed: return QStringLiteral("DatabaseDeleteFailed");
    case ErrorCode::DatabaseTransactionFailed: return QStringLiteral("DatabaseTransactionFailed");
    case ErrorCode::DatabaseCommitFailed: return QStringLiteral("DatabaseCommitFailed");
    case ErrorCode::DatabaseCorruptedData: return QStringLiteral("DatabaseCorruptedData");

    // Service
    case ErrorCode::VariantNotFound: return QStringLiteral("VariantNotFound");
    case ErrorCode::VariantAlreadyExists: return QStringLiteral("VariantAlreadyExists");
    case ErrorCode::VariantSaveFailed: return QStringLiteral("VariantSaveFailed");
    case ErrorCode::VariantLoadFailed: return QStringLiteral("VariantLoadFailed");
    case ErrorCode::VariantRemoveFailed: return QStringLiteral("VariantRemoveFailed");
    case ErrorCode::VariantEmptyContent: return QStringLiteral("VariantEmptyContent");


    // Hash
    case ErrorCode::HashMismatch: return QStringLiteral("HashMismatch");
    case ErrorCode::HashComputeFailed: return QStringLiteral("HashComputeFailed");
    }

    return QStringLiteral("UnknownErrorCode");
}