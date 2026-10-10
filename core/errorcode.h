#ifndef ERRORCODE_H
#define ERRORCODE_H

#include <QString>

enum class ErrorCode {
    None = 0,

    // -- AutomatonData --

    // Invalid variant field
    ADInvalidVariantNumber,

    // Invalid ui-input names
    ADInvalidName,
    ADEmptyName,
    ADDuplicateNameInList,
    ADDuplicateNameInAllList,
    ADCellDuplicateName,

    ADEmptyStateList,
    ADEmptyInputList,
    ADEmptyOutputList,

    ADInitialStateNotSet,
    ADInitialStateNotFound,

    // Tabel cell
    ADInvalidCellIndex,
    ADInvalidCellFormat,
    ADCellNotEditable,

    ADUnknownOutput,
    ADUnknownInput,
    ADUnknownState,

    ADUnusedOutput,
    ADUnusedInput,
    ADUnusedState,

    ADInvalidCellKind,

    // Tabel
    ADTransitionTableSizeMismatch,

    // Automat
    ADUnreachableState,

    // Other
    ADDisableChecker,


    // -- Database --

    // repository
    DatabaseOpenFailed,
    DatabaseQueryFailed,
    DatabaseInsertFailed,
    DatabaseUpdateFailed,
    DatabaseDeleteFailed,
    DatabaseTransactionFailed,
    DatabaseCommitFailed,
    DatabaseCorruptedData,

    DatabasePrepareFailed,
    DatabaseUniqueViolated,
    DatabaseForeignKeyViolated,
    DatabaseConstraintViolated,
    DatabaseConnectionLost,
    DatabaseNoResult,

    DatabaseReadOnly,
    DatabaseAccessDenied,
    DatabaseDiskFull,

    // service
    VariantNotFound,
    VariantAlreadyExists,
    VariantSaveFailed,
    VariantLoadFailed,
    VariantRemoveFailed,
    VariantEmptyContent,

    VariantUnknownState,
    VariantUnknownOutput,



    // -- Hash --

    HashMismatch,
    HashComputeFailed,
};

QString errorCodeToString(ErrorCode code);

#endif // ERRORCODE_H
