#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>

#include <drogon/drogon.h>
#include "qrcodegen.hpp"

#include "QRPDFGenerator.hpp"
#include "PostgresDB.hpp"
#include "uuid.hpp"

static std::time_t portable_timegm(std::tm* tm) {
#if defined(_WIN32)
    return _mkgmtime(tm);
#else
    return timegm(tm);
#endif
}

using namespace drogon;
using namespace qrcodegen;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::function;
using std::runtime_error;
using std::exception;
using drogon::app;

int main() {
    const int myPort = 8080;
    cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << endl;

    // Initialize DB (throws runtime_error on connection failure)

    PostgresDB db("postgresql://postgres.izjgblghemuroxvptbql:NoorHuda123@aws-1-ap-south-1.pooler.supabase.com:5432/postgres");


    // --- Prepared Statements Setup ---
    try {
        db.prepareStatement("insertEvent",
            "INSERT INTO events (id, name, venue, expires_at, optional_details, organization_id, starts_at, ends_at) "
            "VALUES ($1,$2,$3,$4,NULLIF($5, ''),$6,$7,$8);", 8);

        db.prepareStatement("verifyOrg",
            "SELECT id FROM organizations WHERE username = $1 AND password = $2;", 2);

        db.prepareStatement("insertTicket",
            "INSERT INTO tickets (id, event_id, expires_at) VALUES ($1,$2,$3);", 3);

        db.prepareStatement("scanTicket",
            "SELECT status, expires_at FROM tickets WHERE id = $1;", 1);

        db.prepareStatement("redeemTicket",
            "UPDATE tickets SET status='redeemed' WHERE id=$1;", 1);

        db.prepareStatement("getOrgEvents",
            "SELECT id, name FROM events WHERE organization_id=$1;", 1);

        db.prepareStatement("getEventDetails",
            "SELECT name, venue, optional_details, starts_at, ends_at, expires_at FROM events WHERE id=$1;", 1);

        cout << "All prepared statements ready." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;
    }



    // Route: /create-event
    app().registerHandler(
        "/create-event",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {

            auto json = req->getJsonObject();
            Json::Value res;

            // Validate JSON body
            if (!json ||
                !(*json)["username"].isString() ||
                !(*json)["password"].isString() ||
                !(*json)["name"].isString() ||
                !(*json)["venue"].isString() ||
                !(*json)["starts_at"].isString() ||
                !(*json)["ends_at"].isString() ||
                !(*json)["expires_at"].isString() ||
                !(*json)["optional_details"].isString())
            {
                res["success"] = false;
                res["error"] = "Invalid or missing JSON fields";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            // Extract JSON fields
            string username = (*json)["username"].asString();
            string password = (*json)["password"].asString();
            string eventName = (*json)["name"].asString();
            string venue = (*json)["venue"].asString();
            string startsAt = (*json)["starts_at"].asString();
            string endsAt = (*json)["ends_at"].asString();
            string expiresAt = (*json)["expires_at"].asString();

            string optionalDetails;

            if (json->isMember("optional_details") && (*json)["optional_details"].isString()) {
                optionalDetails = (*json)["optional_details"].asString();
            }
            else {
                optionalDetails = ""; // empty string represents NULL
            }


            // Verify organization
            auto rows = db.execPrepared("verifyOrg", { username, password });

            if (rows.empty()) {
                res["success"] = false;
                res["error"] = "Incorrect credentials";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string orgId = rows[0][0];

            // Generate event UUID
            string eventId = generate_uuid();

            // Insert event
            try {
                db.execPrepared("insertEvent",
                    {
                        eventId,
                        eventName,
                        venue,
                        expiresAt,
                        optionalDetails,
                        orgId,
                        startsAt,
                        endsAt
                    }
                );
            }
            catch (const runtime_error& e) {
                res["success"] = false;
                res["error"] = string("Database error: ") + e.what();
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            // Success
            res["success"] = true;
            res["event_id"] = eventId;
            callback(HttpResponse::newHttpJsonResponse(res));
        }
    );

    
    
    app().registerHandler(
        "/create-ticket",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto json = req->getJsonObject();
            Json::Value res;

            if (!json ||
                !(*json)["username"].isString() ||
                !(*json)["password"].isString() ||
                !(*json)["email"].isString() ||
                !(*json)["event_id"].isString()) {
                res["success"] = false;
                res["error"] = "Missing required fields";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string username = (*json)["username"].asString();
            string password = (*json)["password"].asString();
            string email = (*json)["email"].asString();
            string eventId = (*json)["event_id"].asString();


            // Verify organization
            auto rows = db.execPrepared("verifyOrg", { username, password });

            if (rows.empty()) {
                res["success"] = false;
                res["error"] = "Incorrect credentials";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            try {
                // Get expiry from events table
                auto rows = db.execPrepared("getEventDetails", { eventId });
                if (rows.empty()) {
                    res["success"] = false;
                    res["error"] = "Event does not exist";
                    callback(HttpResponse::newHttpJsonResponse(res));
                    return;
                }

                string ticketId = generate_uuid();
                string event_name = rows[0][0];
                string venue = rows[0][1];
                string optional_details = rows[0][2];
                string starts_at = rows[0][3];
                string ends_at = rows[0][4];
                string expires_at = rows[0][5];
                

                try {
                    QRPDFGenerator::generatePDF(ticketId, event_name, venue, optional_details, starts_at, ends_at, "ticket.pdf");
                }
                catch (const exception& e) {
                    res["success"] = false;
                    res["error"] = "PDF generation Failed";
                    callback(HttpResponse::newHttpJsonResponse(res));
                    return;
                }

                db.execPrepared("insertTicket", { ticketId, eventId, expires_at });

                res["success"] = true;
                res["ticket_id"] = ticketId;
            }
            catch (const runtime_error& e) {
                res["success"] = false;
                res["error"] = string("Database error: ") + e.what();
            }

            callback(HttpResponse::newHttpJsonResponse(res));
        }
    );

    // ---- Events route ----
    app().registerHandler(
        "/mobile/events",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto json = req->getJsonObject();
            Json::Value res;

            if (!json || !(*json)["id"].isString()) {
                res["success"] = false;
                res["error"] = "No user ID provided";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }
            
            string orgId = (*json)["id"].asString();
            res["success"] = true;

            // Query events for this user
            auto rows = db.execPrepared("getOrgEvents", { orgId });

            Json::Value events(Json::arrayValue);
            for (auto& row : rows) {
                Json::Value e;
                e["id"] = row[0];      // event id
                e["name"] = row[1];                // event name
                events.append(e);
            }

            res["events"] = events;
            

            callback(HttpResponse::newHttpJsonResponse(res));
        
        }
    );

    // ---- Ticket scan route ----
    app().registerHandler(
        "/scan",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto json = req->getJsonObject();
            Json::Value res;

            if (!json || !(*json)["ticketId"].isString()) {
                res["result_status"] = "error";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string ticketId = (*json)["ticketId"].asString();

            auto rows = db.execPrepared("scanTicket", { ticketId });

            if (rows.empty()) {
                res["result_status"] = "noExist";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string expiresAt = rows[0][1];

            // Remove timezone offset (+00, +05, etc.)
            string expiresAtClean = expiresAt;
            size_t plusPos = expiresAtClean.find('+');
            if (plusPos != string::npos) {
                expiresAtClean = expiresAtClean.substr(0, plusPos);
            }

            // Parse datetime (no timezone)
            std::tm tm = {};
            std::istringstream ss(expiresAtClean);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

            bool isExpired = false;
            if (!ss.fail()) {
                std::time_t expiryTimeT = portable_timegm(&tm);
                auto expiryTime = std::chrono::system_clock::from_time_t(expiryTimeT);
                auto now = std::chrono::system_clock::now();

                if (now > expiryTime) {
                    isExpired = true;
                }
            }
        
            string ticketStatus = isExpired ? "expired" : rows[0][0];

            if (ticketStatus == "active") {

                try {
                    db.execPreparedCommand("redeemTicket", { ticketId });
                    res["result_status"] = "redeemed";
                }
                catch (const std::runtime_error&) {
                    res["result_status"] = "error";
                }
            }
            else if (ticketStatus == "expired") {
                res["result_status"] = "expired";
            }
            else {
                res["result_status"] = "noExist";
            }
            

            callback(HttpResponse::newHttpJsonResponse(res));
        }
    );

    app().registerHandler(
        "/auth",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto json = req->getJsonObject();
            Json::Value res;

            // Validate JSON body
            if (!json || !(*json)["username"].isString() || !(*json)["password"].isString()) {
                res["success"] = false;
                res["error"] = "Invalid JSON or missing fields";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string username = (*json)["username"].asString();
            string password = (*json)["password"].asString();

            try {
                // Verify organization
                auto rows = db.execPrepared("verifyOrg", { username, password });

                if (rows.empty()) {
                    res["success"] = false;
                    res["error"] = "Incorrect credentials";
                }
                else {
                    string orgId = rows[0][0];  // Already a string in your wrapper
                    res["success"] = true;
                    res["id"] = orgId;
                }
            }
            catch (const runtime_error& e) {
                res["success"] = false;
                res["error"] = string("Database error: ") + e.what();
            }

            callback(HttpResponse::newHttpJsonResponse(res));
        }
    );

    drogon::app().addListener("0.0.0.0", myPort);
    drogon::app().run();

    return 0;
}
