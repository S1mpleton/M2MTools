#include "translateresult.h"

#include <QCoreApplication>

namespace {
    [[maybe_unused]] const char* const kResultTranslationSources[] = {
        QT_TRANSLATE_NOOP("Result", "Failed to delete data from database."),
        QT_TRANSLATE_NOOP("Result", "Database transaction failed."),
        QT_TRANSLATE_NOOP("Result", "Database contains corrupted data. Please contact the administrator."),
        QT_TRANSLATE_NOOP("Result", "The archive contains an invalid variant entry."),
        QT_TRANSLATE_NOOP("Result", "The archive contains an invalid variant number or type."),
        QT_TRANSLATE_NOOP("Result", "The archive contains invalid symbol lists."),
        QT_TRANSLATE_NOOP("Result", "The archive does not contain a transition table."),
        QT_TRANSLATE_NOOP("Result", "The archive transition table is corrupted."),
        QT_TRANSLATE_NOOP("Result", "Variant #%1 in the archive has no image."),
        QT_TRANSLATE_NOOP("Result", "A variant in the archive failed validation."),
        QT_TRANSLATE_NOOP("Result", "Failed to prepare the BazaVar file."),
        QT_TRANSLATE_NOOP("Result", "Failed to open the BazaVar archive file."),
        QT_TRANSLATE_NOOP("Result", "Failed to save the BazaVar archive."),
        QT_TRANSLATE_NOOP("Result", "Internal error: archive data destination was not provided."),
        QT_TRANSLATE_NOOP("Result", "The selected file is not an M2MTools archive."),
        QT_TRANSLATE_NOOP("Result", "The BazaVar file is damaged or was created by an incompatible application."),
        QT_TRANSLATE_NOOP("Result", "The BazaVar file data is corrupted."),
        QT_TRANSLATE_NOOP("Result", "This BazaVar archive version or format is not supported."),
        QT_TRANSLATE_NOOP("Result", "The archive contains duplicate variant number #%1."),
        QT_TRANSLATE_NOOP("Result", "Database integrity check failed. The data may have been modified manually."),
        QT_TRANSLATE_NOOP("Result", "Failed to compute content hash."),
        QT_TRANSLATE_NOOP("Result", "The operation could not be completed. See the event log for details."),
    };

    QString trResult(const char* text) {

        return QCoreApplication::translate("Result", text);
    }
}

QString TranslateResult::forUi(const Result& r) {
    using EC = ErrorCode;

    switch (r.code()) {

    // -- AutomatonData --
    case EC::ADEmptyStateList:
        return QCoreApplication::translate("AutomatonData",
            "Automaton has no states. "
            "Please add at least one state.");

    case EC::ADEmptyInputList:
        return QCoreApplication::translate("AutomatonData",
                        "Automaton has no input signals. "
                        "Please add at least one input signal.");

    case EC::ADEmptyOutputList:
        return QCoreApplication::translate("AutomatonData",
                        "Automaton has no output signals. "
                        "Please add at least one output signal.");

    case EC::ADTransitionTableSizeMismatch:
        return QCoreApplication::translate("AutomatonData",
                        "Internal error: transition table size "
                        "does not match the number of input signals.");

    case EC::ADStateWithoutIncomingTransition:
        return QCoreApplication::translate("AutomatonData",
            "State '%1' has no incoming transition. "
            "Add a transition into this state.")
            .arg(r.offender());

    case EC::ADStateWithoutOutgoingTransition:
        return QCoreApplication::translate("AutomatonData",
            "State '%1' has no outgoing transition. "
            "Add a transition from this state.")
            .arg(r.offender());

    case EC::ADUnusedInput:
        return QCoreApplication::translate("AutomatonData",
                        "Input signal '%1' is not used in any transition.")
            .arg(r.offender());

    case EC::ADUnusedOutput:
        return QCoreApplication::translate("AutomatonData",
                        "Output signal '%1' is not used.")
            .arg(r.offender());

    case EC::ADDuplicateNameInList:
        return QCoreApplication::translate("AutomatonData",
                        "There are duplicates of '%1' name in this list.")
            .arg(r.offender());

    case EC::ADDuplicateNameInAllList:
        return QCoreApplication::translate("AutomatonData",
                        "The name '%1' already exist in other list.")
            .arg(r.offender());

    case EC::ADCellDuplicateName:
        return QCoreApplication::translate("AutomatonData",
                        "The cell has duplicate name '%1'.")
            .arg(r.offender());

    case EC::ADInvalidName:
        return QCoreApplication::translate("AutomatonData",
                        "Invalid name: '%1'. "
                        "Only letters and digits are allowed, "
                        "first character must be a letter.")
            .arg(r.offender());

    case EC::ADEmptyName:
        return QCoreApplication::translate("AutomatonData",
                        "Name cannot be empty.");

    case EC::ADInitialStateNotSet:
        return QCoreApplication::translate("AutomatonData",
                        "Initial state is not set.");

    case EC::ADInitialStateNotFound:
        return QCoreApplication::translate("AutomatonData",
                        "Initial state '%1' not found in state list.")
            .arg(r.offender());

    case EC::ADUnknownState:
        return QCoreApplication::translate("AutomatonData",
                        "Unknown state '%1'.")
            .arg(r.offender());

    case EC::ADUnknownOutput:
        return QCoreApplication::translate("AutomatonData",
                        "Unknown output signal '%1'.")
            .arg(r.offender());

    case EC::ADUnknownInput:
        return QCoreApplication::translate("AutomatonData",
                        "Unknown input signal '%1'.")
            .arg(r.offender());



    // -- Database --
    case EC::DatabaseOpenFailed:
        return QCoreApplication::translate("Database",
                            "Failed to open database.");

    case EC::DatabaseQueryFailed:
        return QCoreApplication::translate("Database",
                        "Database query failed. "
                        "See logs for details.");

    case EC::DatabaseInsertFailed:
        return QCoreApplication::translate("Database",
                        "Failed to insert data into database.");

    case EC::DatabaseUpdateFailed:
        return QCoreApplication::translate("Database",
                        "Failed to update data in database.");

    case EC::DatabaseDeleteFailed:
        return trResult("Failed to delete data from database.");

    case EC::DatabaseTransactionFailed:
        return trResult("Database transaction failed.");

    case EC::DatabaseCommitFailed:
        return QCoreApplication::translate("Database",
                        "Failed to commit database changes.");

    case EC::DatabaseCorruptedData:
        return trResult("Database contains corrupted data. "
                        "Please contact the administrator.");


    // -- Service --
    case EC::VariantNotFound:
        return QCoreApplication::translate("VariantService",
                        "Variant №%1 not found.").arg(r.offender());

    case EC::VariantAlreadyExists:
        return QCoreApplication::translate("VariantService",
                        "Variant №%1 already exists.").arg(r.offender());

    case EC::VariantSaveFailed:
        return QCoreApplication::translate("VariantService",
                        "Failed to save variant.");

    case EC::VariantLoadFailed:
        return QCoreApplication::translate("VariantService",
                        "Failed to load variant.");

    case EC::VariantRemoveFailed:
        return QCoreApplication::translate("VariantService",
                        "Failed to remove variant.");

    case EC::VariantEmptyContent:
        return QCoreApplication::translate("VariantService",
                        "Variant has no content (empty states or inputs).");
    case EC::VariantMissingImage:
        return QCoreApplication::translate("VariantService",
                        "Variant #%1 has no task image.").arg(r.offender());

    // -- Archive --
    case EC::ArchiveInvalidEntry:
        return trResult("The archive contains an invalid variant entry.");
    case EC::ArchiveInvalidMetadata:
        return trResult("The archive contains an invalid variant number or type.");
    case EC::ArchiveInvalidSymbols:
        return trResult("The archive contains invalid symbol lists.");
    case EC::ArchiveMissingTransitionTable:
        return trResult("The archive does not contain a transition table.");
    case EC::ArchiveCorruptedTransitionTable:
        return trResult("The archive transition table is corrupted.");
    case EC::ArchiveMissingImage:
        return trResult("Variant #%1 in the archive has no image.")
            .arg(r.offender());
    case EC::ArchiveInvalidVariant:
        return trResult("A variant in the archive failed validation.");
    case EC::ArchiveEncryptionFailed:
        return trResult("Failed to prepare the BazaVar file.");
    case EC::ArchiveFileOpenFailed:
        return trResult("Failed to open the BazaVar archive file.");
    case EC::ArchiveSaveFailed:
        return trResult("Failed to save the BazaVar archive.");
    case EC::ArchiveInvalidTarget:
        return trResult("Internal error: archive data destination was not provided.");
    case EC::ArchiveInvalidFormat:
        return trResult("The selected file is not an M2MTools archive.");
    case EC::ArchiveDecryptionFailed:
        return trResult("The BazaVar file is damaged or was created by an incompatible application.");
    case EC::ArchiveCorruptedPayload:
        return trResult("The BazaVar file data is corrupted.");
    case EC::ArchiveUnsupportedVersion:
        return trResult("This BazaVar archive version or format is not supported.");
    case EC::ArchiveDuplicateVariantNumber:
        return trResult("The archive contains duplicate variant number #%1.")
            .arg(r.offender());


    // -- Hash --
    case EC::HashMismatch:
        return trResult("Database integrity check failed. "
                        "The data may have been modified manually.");

    case EC::HashComputeFailed:
        return trResult("Failed to compute content hash.");

    // -- Fallback --

    case EC::None:
    {
        const QByteArray source = r.message().toUtf8();
        return QCoreApplication::translate("ResultMessage", source.constData());
    }

    default:
        return trResult("The operation could not be completed. "
                        "See the event log for details.");
    }
}

QString TranslateResult::forLog(const Result& result) {
    QString text = result.message().isEmpty()
        ? QStringLiteral("Operation completed without a diagnostic message")
        : result.message();
    if (result.code() != ErrorCode::None)
        text += QStringLiteral(" | code=%1").arg(errorCodeToString(result.code()));
    if (!result.offender().isEmpty())
        text += QStringLiteral(" | offender=%1").arg(result.offender());
    if (!result.details().isEmpty())
        text += QStringLiteral(" | details=%1").arg(result.details());
    return text;
}

QString translateResult(const Result& result) {
    return TranslateResult::forUi(result);
}
