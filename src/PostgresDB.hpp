#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <libpq-fe.h>

/**
 * @brief Modernized C++ PostgreSQL wrapper (libpq)
 *
 * Features:
 * - Throws std::runtime_error on any error
 * - RAII-safe PGresult handling
 * - Separate checks for command vs query results
 * - NULL-safe query results
 * - Prepared statement support for SELECT and non-SELECT
 * - Transaction RAII class
 */
class PostgresDB {
private:
    PGconn* conn;

    // -----------------------------
    // RAII wrapper for PGresult*
    // -----------------------------
    struct PGResultPtr {
        PGresult* res;
        explicit PGResultPtr(PGresult* r) : res(r) {}
        ~PGResultPtr() { if (res) PQclear(res); }
        PGresult* get() const { return res; }
    };

    // -----------------------------
    // Helpers to check results
    // -----------------------------
    void checkCommand(PGresult* res, const std::string& context) {
        if (!res) throw std::runtime_error(context + ": Result is null");
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string msg = PQresultErrorMessage(res);
            throw std::runtime_error(context + ": " + msg);
        }
    }

    void checkQuery(PGresult* res, const std::string& context) {
        if (!res) throw std::runtime_error(context + ": Result is null");
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string msg = PQresultErrorMessage(res);
            throw std::runtime_error(context + ": " + msg);
        }
    }

    // -----------------------------
    // Convert PGresult to vector
    // -----------------------------
    std::vector<std::vector<std::string>> resultToVector(PGresult* res) {
        int nRows = PQntuples(res);
        int nCols = PQnfields(res);
        std::vector<std::vector<std::string>> results(nRows, std::vector<std::string>(nCols));

        for (int i = 0; i < nRows; ++i) {
            for (int j = 0; j < nCols; ++j) {
                if (PQgetisnull(res, i, j))
                    results[i][j] = "NULL";
                else
                    results[i][j] = PQgetvalue(res, i, j);
            }
        }
        return results;
    }

public:
    // -----------------------------
    // Constructor & Destructor
    // -----------------------------
    explicit PostgresDB(const std::string& conninfo) {
        conn = PQconnectdb(conninfo.c_str());
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            std::string msg = "Connection failed: " + std::string(PQerrorMessage(conn));
            if (conn) PQfinish(conn);
            throw std::runtime_error(msg);
        }
    }

    ~PostgresDB() {
        if (conn) PQfinish(conn);
    }

    // -----------------------------
    // Transaction helpers
    // -----------------------------
    void beginTransaction() { execCommand("BEGIN;"); }
    void commitTransaction() { execCommand("COMMIT;"); }
    void rollbackTransaction() { execCommand("ROLLBACK;"); }

    // -----------------------------
    // Execute non-SELECT SQL command
    // -----------------------------
    void execCommand(const std::string& sql) {
        PGResultPtr res(PQexec(conn, sql.c_str()));
        checkCommand(res.get(), "execCommand");
    }

    // -----------------------------
    // Execute SELECT query
    // -----------------------------
    std::vector<std::vector<std::string>> query(const std::string& sql) {
        PGResultPtr res(PQexec(conn, sql.c_str()));
        checkQuery(res.get(), "query");
        return resultToVector(res.get());
    }

    // -----------------------------
    // Prepare a statement
    // -----------------------------
    void prepareStatement(const std::string& name, const std::string& sql, int nParams) {
        PGResultPtr res(PQprepare(conn, name.c_str(), sql.c_str(), nParams, nullptr));
        checkCommand(res.get(), "prepareStatement");
    }

    // -----------------------------
    // Execute prepared SELECT statement
    // -----------------------------
    std::vector<std::vector<std::string>> execPrepared(
        const std::string& stmtName,
        const std::vector<std::string>& params
    ) {
        std::vector<const char*> cparams;
        for (const auto& p : params) cparams.push_back(p.c_str());

        PGResultPtr res(PQexecPrepared(conn, stmtName.c_str(),
            static_cast<int>(cparams.size()),
            cparams.data(), nullptr, nullptr, 0));

        ExecStatusType status = PQresultStatus(res.get());
        if (status == PGRES_TUPLES_OK)
            return resultToVector(res.get());
        else if (status == PGRES_COMMAND_OK)
            return {}; // empty vector for non-SELECT
        else
            throw std::runtime_error("execPrepared: " + std::string(PQresultErrorMessage(res.get())));
    }

    // -----------------------------
    // Execute prepared non-SELECT statement
    // -----------------------------
    void execPreparedCommand(
        const std::string& stmtName,
        const std::vector<std::string>& params
    ) {
        std::vector<const char*> cparams;
        for (const auto& p : params) cparams.push_back(p.c_str());

        PGResultPtr res(PQexecPrepared(conn, stmtName.c_str(),
            static_cast<int>(cparams.size()),
            cparams.data(), nullptr, nullptr, 0));
        checkCommand(res.get(), "execPreparedCommand");
    }

    // -----------------------------
    // Check if connection is alive
    // -----------------------------
    bool isConnected() const { return conn && PQstatus(conn) == CONNECTION_OK; }

    // -----------------------------
    // RAII Transaction class
    // -----------------------------
    class Transaction {
    private:
        PostgresDB& db;
        bool committed;
    public:
        explicit Transaction(PostgresDB& database) : db(database), committed(false) {
            db.beginTransaction();
        }

        void commit() {
            if (!committed) {
                db.commitTransaction();
                committed = true;
            }
        }

        ~Transaction() {
            if (!committed) {
                try {
                    db.rollbackTransaction();
                }
                catch (...) {
                    // Prevent throwing in destructor
                }
            }
        }

        // Disable copying
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        // Enable moving
        Transaction(Transaction&& other) noexcept : db(other.db), committed(other.committed) {
            other.committed = true;
        }
        Transaction& operator=(Transaction&& other) noexcept {
            if (this != &other) {
                if (!committed) db.rollbackTransaction();
                db = other.db;
                committed = other.committed;
                other.committed = true;
            }
            return *this;
        }
    };
};
