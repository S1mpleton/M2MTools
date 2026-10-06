#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "data/database.h"

class Transaction {
public:
    explicit Transaction(Database* db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    bool commit(QString* error = nullptr);
    void rollback();

private:
    Database* m_db;
    bool m_finished = false;
};

#endif // TRANSACTION_H
