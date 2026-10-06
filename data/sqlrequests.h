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
}

#endif // SQLREQUESTS_H
