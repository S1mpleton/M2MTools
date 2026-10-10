#include "translateresult.h"

#include <QCoreApplication>

namespace {
    QString trResult(const char* text) {

        return QCoreApplication::translate("Result", text);
    }
}

QString translateResult(const Result& r) {
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

    case EC::ADUnusedState:
        return QCoreApplication::translate("AutomatonData",
                        "State '%1' is not used in any transition.")
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


    // -- Hash --
    case EC::HashMismatch:
        return trResult("Database integrity check failed. "
                        "The data may have been modified manually.");

    case EC::HashComputeFailed:
        return trResult("Failed to compute content hash.");

    // -- Fallback --

    case EC::None:
        return r.message();

    default:
        return "(No translate) " + r.message();
    }
}