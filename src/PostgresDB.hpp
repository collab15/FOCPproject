#pragma once
#include <string>
#include <vector>
#include <libpq-fe.h>
#include <stdexcept>

/**
 * @brief Modern C++ PostgreSQL wrapper using libpq.
 *
 * - Throws std::runtime_error automatically only if connection fails.
 * - All other operations (query, exec, transactions, prepared statements) throw exceptions
 *   that are fully catchable in main().
 * - Returns results as std::vector<std::vector<std::string>>, no C-style strings.
 */
class PostgresDB {
private:
    PGconn* conn;

    // Helper to check result status and throw std::runtime_error if failed
    void checkResult(PGresult* res, ExecStatusType expectedStatus, const std::string& context) {
        if (!res)
            throw std::runtime_error(context + ": Result is null");
        if (PQresultStatus(res) != expectedStatus) {
            std::string msg = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error(context + ": " + msg);
        }
    }

    // Convert PGresult to vector of vector of strings
    std::vector<std::vector<std::string>> resultToVector(PGresult* res) {
        int nRows = PQntuples(res);
        int nCols = PQnfields(res);
        std::vector<std::vector<std::string>> results(nRows, std::vector<std::string>(nCols));
        for (int i = 0; i < nRows; ++i)
            for (int j = 0; j < nCols; ++j)
                results[i][j] = PQgetvalue(res, i, j);
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
            std::cerr << "PQ Error: " << PQerrorMessage(conn) << std::endl;

            PQfinish(conn);
            throw std::runtime_error(msg); // Only automatic runtime_error here
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
    // Execute a non-SELECT SQL command (INSERT, UPDATE, DELETE, etc.)
    // Throws exception on failure
    // -----------------------------
    void execCommand(const std::string& sql) {
        PGresult* res = PQexec(conn, sql.c_str());
        checkResult(res, PGRES_COMMAND_OK, "execCommand");
        PQclear(res);
    }

    // -----------------------------
    // Execute a SELECT query
    // Throws exception on failure
    // -----------------------------
    std::vector<std::vector<std::string>> query(const std::string& sql) {
        PGresult* res = PQexec(conn, sql.c_str());
        checkResult(res, PGRES_TUPLES_OK, "query");
        auto results = resultToVector(res);
        PQclear(res);
        return results;
    }

    // -----------------------------
    // Prepare a statement
    // Throws exception on failure
    // -----------------------------
    void prepareStatement(const std::string& name, const std::string& sql, int nParams) {
        PGresult* res = PQprepare(conn, name.c_str(), sql.c_str(), nParams, nullptr);
        checkResult(res, PGRES_COMMAND_OK, "prepareStatement");
        PQclear(res);
    }

    // -----------------------------
    // Execute a prepared statement
    // Throws exception on failure
    // -----------------------------
    std::vector<std::vector<std::string>> execPrepared(
        const std::string& stmtName,
        const std::vector<std::string>& params
    ) {
        std::vector<const char*> cparams;
        for (const auto& p : params) cparams.push_back(p.c_str());

        PGresult* res = PQexecPrepared(conn, stmtName.c_str(),
            static_cast<int>(cparams.size()),
            cparams.data(), nullptr, nullptr, 0);
        checkResult(res, PGRES_TUPLES_OK, "execPrepared");
        auto results = resultToVector(res);
        PQclear(res);
        return results;
    }

    // -----------------------------
    // Check if connection is alive
    // -----------------------------
    bool isConnected() const { return conn && PQstatus(conn) == CONNECTION_OK; }
};
