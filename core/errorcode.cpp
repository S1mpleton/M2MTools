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
    case ErrorCode::ADStateWithoutIncomingTransition:
        return QStringLiteral("StateWithoutIncomingTransition");
    case ErrorCode::ADStateWithoutOutgoingTransition:
        return QStringLiteral("StateWithoutOutgoingTransition");
    case ErrorCode::ADUnusedInput: return QStringLiteral("UnusedInput");
    case ErrorCode::ADUnusedOutput: return QStringLiteral("UnusedOutput");

    case ErrorCode::ADInvalidVariantNumber: return QStringLiteral("InvalidVariantNumber");
    case ErrorCode::ADInvalidName: return QStringLiteral("InvalidName");
    case ErrorCode::ADEmptyName: return QStringLiteral("EmptyName");
    case ErrorCode::ADDuplicateNameInList: return QStringLiteral("DuplicateNameInList");
    case ErrorCode::ADDuplicateNameInAllList: return QStringLiteral("DuplicateNameAcrossLists");
    case ErrorCode::ADCellDuplicateName: return QStringLiteral("CellDuplicateName");
    case ErrorCode::ADInitialStateNotSet: return QStringLiteral("InitialStateNotSet");
    case ErrorCode::ADInitialStateNotFound: return QStringLiteral("InitialStateNotFound");
    case ErrorCode::ADInvalidCellIndex: return QStringLiteral("InvalidCellIndex");
    case ErrorCode::ADInvalidCellFormat: return QStringLiteral("InvalidCellFormat");
    case ErrorCode::ADCellNotEditable: return QStringLiteral("CellNotEditable");
    case ErrorCode::ADUnknownOutput: return QStringLiteral("UnknownOutput");
    case ErrorCode::ADUnknownInput: return QStringLiteral("UnknownInput");
    case ErrorCode::ADUnknownState: return QStringLiteral("UnknownState");
    case ErrorCode::ADInvalidCellKind: return QStringLiteral("InvalidCellKind");

    // Database
    case ErrorCode::DatabaseOpenFailed: return QStringLiteral("DatabaseOpenFailed");
    case ErrorCode::DatabaseQueryFailed: return QStringLiteral("DatabaseQueryFailed");
    case ErrorCode::DatabaseInsertFailed: return QStringLiteral("DatabaseInsertFailed");
    case ErrorCode::DatabaseUpdateFailed: return QStringLiteral("DatabaseUpdateFailed");
    case ErrorCode::DatabaseDeleteFailed: return QStringLiteral("DatabaseDeleteFailed");
    case ErrorCode::DatabaseTransactionFailed: return QStringLiteral("DatabaseTransactionFailed");
    case ErrorCode::DatabaseCommitFailed: return QStringLiteral("DatabaseCommitFailed");
    case ErrorCode::DatabaseCorruptedData: return QStringLiteral("DatabaseCorruptedData");
    case ErrorCode::DatabasePrepareFailed: return QStringLiteral("DatabasePrepareFailed");
    case ErrorCode::DatabaseUniqueViolated: return QStringLiteral("DatabaseUniqueViolated");
    case ErrorCode::DatabaseForeignKeyViolated: return QStringLiteral("DatabaseForeignKeyViolated");
    case ErrorCode::DatabaseConstraintViolated: return QStringLiteral("DatabaseConstraintViolated");
    case ErrorCode::DatabaseConnectionLost: return QStringLiteral("DatabaseConnectionLost");
    case ErrorCode::DatabaseNoResult: return QStringLiteral("DatabaseNoResult");
    case ErrorCode::DatabaseReadOnly: return QStringLiteral("DatabaseReadOnly");
    case ErrorCode::DatabaseAccessDenied: return QStringLiteral("DatabaseAccessDenied");
    case ErrorCode::DatabaseDiskFull: return QStringLiteral("DatabaseDiskFull");

    // Service
    case ErrorCode::VariantNotFound: return QStringLiteral("VariantNotFound");
    case ErrorCode::VariantAlreadyExists: return QStringLiteral("VariantAlreadyExists");
    case ErrorCode::VariantSaveFailed: return QStringLiteral("VariantSaveFailed");
    case ErrorCode::VariantLoadFailed: return QStringLiteral("VariantLoadFailed");
    case ErrorCode::VariantRemoveFailed: return QStringLiteral("VariantRemoveFailed");
    case ErrorCode::VariantEmptyContent: return QStringLiteral("VariantEmptyContent");
    case ErrorCode::VariantMissingImage: return QStringLiteral("VariantMissingImage");
    case ErrorCode::VariantUnknownState: return QStringLiteral("VariantUnknownState");
    case ErrorCode::VariantUnknownOutput: return QStringLiteral("VariantUnknownOutput");

    // Archive
    case ErrorCode::ArchiveInvalidEntry: return QStringLiteral("ArchiveInvalidEntry");
    case ErrorCode::ArchiveInvalidMetadata: return QStringLiteral("ArchiveInvalidMetadata");
    case ErrorCode::ArchiveInvalidSymbols: return QStringLiteral("ArchiveInvalidSymbols");
    case ErrorCode::ArchiveMissingTransitionTable: return QStringLiteral("ArchiveMissingTransitionTable");
    case ErrorCode::ArchiveCorruptedTransitionTable: return QStringLiteral("ArchiveCorruptedTransitionTable");
    case ErrorCode::ArchiveMissingImage: return QStringLiteral("ArchiveMissingImage");
    case ErrorCode::ArchiveInvalidVariant: return QStringLiteral("ArchiveInvalidVariant");
    case ErrorCode::ArchiveEncryptionFailed: return QStringLiteral("ArchiveEncryptionFailed");
    case ErrorCode::ArchiveFileOpenFailed: return QStringLiteral("ArchiveFileOpenFailed");
    case ErrorCode::ArchiveSaveFailed: return QStringLiteral("ArchiveSaveFailed");
    case ErrorCode::ArchiveInvalidTarget: return QStringLiteral("ArchiveInvalidTarget");
    case ErrorCode::ArchiveInvalidFormat: return QStringLiteral("ArchiveInvalidFormat");
    case ErrorCode::ArchiveDecryptionFailed: return QStringLiteral("ArchiveDecryptionFailed");
    case ErrorCode::ArchiveCorruptedPayload: return QStringLiteral("ArchiveCorruptedPayload");
    case ErrorCode::ArchiveUnsupportedVersion: return QStringLiteral("ArchiveUnsupportedVersion");
    case ErrorCode::ArchiveDuplicateVariantNumber: return QStringLiteral("ArchiveDuplicateVariantNumber");


    // Hash
    case ErrorCode::HashMismatch: return QStringLiteral("HashMismatch");
    case ErrorCode::HashComputeFailed: return QStringLiteral("HashComputeFailed");
    }

    return QStringLiteral("UnknownErrorCode");
}
