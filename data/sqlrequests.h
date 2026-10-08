#ifndef SQLREQUESTS_H
#define SQLREQUESTS_H

#include <QString>

namespace SQLRequests {

    // Create Tables
    inline const QString CREATE_VARIANT_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS Variants ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "number INTEGER NOT NULL UNIQUE, "
            "conversion TEXT NOT NULL "
                "CHECK (conversion IN ('MealyToMoore', 'MooreToMealy'))"
        ");"
    );

    inline const QString CREATE_INPUT_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS Inputs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "variant_id INTEGER NOT NULL, "

            "name TEXT NOT NULL, "

            "FOREIGN KEY (variant_id) REFERENCES Variants(id) ON DELETE CASCADE"
        ");"
    );

    inline const QString CREATE_OUTPUTS_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS Outputs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "variant_id INTEGER NOT NULL, "

            "name TEXT NOT NULL, "

            "FOREIGN KEY (variant_id) REFERENCES Variants(id) ON DELETE CASCADE"
        ");"
    );

    inline const QString CREATE_STATE_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS States ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "variant_id INTEGER NOT NULL, "
            "is_init INTEGER NOT NULL DEFAULT 0, "

            "name TEXT NOT NULL, "

            "FOREIGN KEY (variant_id) REFERENCES Variants(id) ON DELETE CASCADE"
        ");"
    );

    inline const QString CREATE_TRANSITION_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS Transitions ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "variant_id INTEGER NOT NULL, "

            "from_state_id INTEGER NOT NULL, "
            "to_state_id INTEGER, "
            "input_signal_id INTEGER NOT NULL, "

            "UNIQUE (variant_id, from_state_id, input_signal_id), "
            "FOREIGN KEY (variant_id) REFERENCES Variants(id) ON DELETE CASCADE, "
            "FOREIGN KEY (from_state_id) REFERENCES States(id), "
            "FOREIGN KEY (to_state_id) REFERENCES States(id), "
            "FOREIGN KEY (input_signal_id) REFERENCES Inputs(id)"
        ");"
    );

    inline const QString CREATE_MEALY_TRANSITION_OUTPUTS_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS MealyTransitionOutputs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "

            "transition_id INTEGER NOT NULL, "
            "output_id INTEGER NOT NULL, "

            "UNIQUE (transition_id, output_id), "
            "FOREIGN KEY (transition_id) REFERENCES Transitions(id) ON DELETE CASCADE, "
            "FOREIGN KEY (output_id) REFERENCES Outputs(id) ON DELETE CASCADE"
        ");"
    );

    inline const QString CREATE_MOORE_STATE_OUTPUTS_TABLE = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS MooreStateOutputs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "

            "state_id INTEGER NOT NULL, "
            "output_id INTEGER NOT NULL, "

            "UNIQUE (state_id, output_id), "
            "FOREIGN KEY (state_id) REFERENCES States(id) ON DELETE CASCADE, "
            "FOREIGN KEY (output_id) REFERENCES Outputs(id) ON DELETE CASCADE"
        ");"
    );

    // Others
    inline const QString CREATE_UNIQUE_INIT_STATE_INDEX = QStringLiteral(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_one_init_per_variant "
            "ON States(variant_id) WHERE is_init = 1;"
    );

    // Variants
    inline const QString SELECT_VARIANT_BY_NUMBER_AND_TYPE = QStringLiteral(
        "SELECT id, number, conversion "
        "FROM Variants "
        "WHERE number = :num"
        );

    inline const QString SELECT_ALL_VARIANTS = QStringLiteral(
        "SELECT id, number, conversion "
        "FROM Variants "
        "ORDER BY conversion, number"
        );

    inline const QString SELECT_VARIANTS_BY_TYPE = QStringLiteral(
        "SELECT id, number, conversion "
        "FROM Variants "
        "WHERE conversion = :conv "
        "ORDER BY number"
        );

    inline const QString INSERT_VARIANT = QStringLiteral(
        "INSERT INTO Variants (number, conversion) "
        "VALUES (:num, :conv)"
        );

    inline const QString DELETE_VARIANT = QStringLiteral(
        "DELETE FROM Variants WHERE id = :id"
        );

    // States
    inline const QString SELECT_STATES_BY_VARIANT = QStringLiteral(
        "SELECT id, variant_id, name, is_init "
        "FROM States "
        "WHERE variant_id = :vid "
        "ORDER BY id"
        );

    inline const QString INSERT_STATE = QStringLiteral(
        "INSERT INTO States (variant_id, name, is_init) "
        "VALUES (:vid, :name, :init)"
        );

    // Inputs
    inline const QString SELECT_INPUTS_BY_VARIANT = QStringLiteral(
        "SELECT id, variant_id, name "
        "FROM Inputs "
        "WHERE variant_id = :vid "
        "ORDER BY id"
        );

    inline const QString INSERT_INPUT = QStringLiteral(
        "INSERT INTO Inputs (variant_id, name) "
        "VALUES (:vid, :name)"
        );

    // Outputs
    inline const QString SELECT_OUTPUTS_BY_VARIANT = QStringLiteral(
        "SELECT id, variant_id, name "
        "FROM Outputs "
        "WHERE variant_id = :vid "
        "ORDER BY id"
        );

    inline const QString INSERT_OUTPUT = QStringLiteral(
        "INSERT INTO Outputs (variant_id, name) "
        "VALUES (:vid, :name)"
        );

    // Transitions
    inline const QString SELECT_TRANSITIONS_BY_VARIANT = QStringLiteral(
        "SELECT id, variant_id, from_state_id, "
        "       to_state_id, input_signal_id "
        "FROM Transitions "
        "WHERE variant_id = :vid "
        "ORDER BY id"
        );

    inline const QString INSERT_TRANSITION = QStringLiteral(
        "INSERT INTO Transitions "
        "    (variant_id, from_state_id, to_state_id, input_signal_id) "
        "VALUES (:vid, :from, :to, :input)"
        );

    // Mealy outputs
    inline const QString SELECT_MEALY_OUTPUTS_BY_TRANSITION = QStringLiteral(
        "SELECT transition_id, output_id "
        "FROM MealyTransitionOutputs "
        "WHERE transition_id = :tid "
        "ORDER BY output_id"
        );

    inline const QString SELECT_MEALY_OUTPUTS_BY_VARIANT = QStringLiteral(
        "SELECT mto.transition_id, mto.output_id "
        "FROM MealyTransitionOutputs mto "
        "JOIN Transitions t ON t.id = mto.transition_id "
        "WHERE t.variant_id = :vid"
        );

    inline const QString INSERT_MEALY_OUTPUT = QStringLiteral(
        "INSERT INTO MealyTransitionOutputs (transition_id, output_id) "
        "VALUES (:tid, :oid)"
        );

    // Moore outputs
    inline const QString SELECT_MOORE_OUTPUTS_BY_VARIANT = QStringLiteral(
        "SELECT mso.state_id, mso.output_id "
        "FROM MooreStateOutputs mso "
        "JOIN States s ON s.id = mso.state_id "
        "WHERE s.variant_id = :vid "
        "ORDER BY mso.state_id, mso.output_id"
        );

    inline const QString INSERT_MOORE_OUTPUT = QStringLiteral(
        "INSERT INTO MooreStateOutputs (state_id, output_id) "
        "VALUES (:sid, :oid)"
        );
}

#endif // SQLREQUESTS_H
