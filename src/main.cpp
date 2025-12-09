#include <iostream>
#include <drogon/drogon.h>
#include "qrcodegen.hpp"

#include "QRPDFGenerator.hpp"
#include "PostgresDB.hpp"
#include "uuid.hpp"

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


    // Prepare insert statement once (throws on failure)
    try {
        db.prepareStatement("insertEvent",
            "INSERT INTO events (id, name, venue, expires_at, optional_details, organization_id, starts_at, ends_at) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8);",
            8);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;  // Stop program
    }

    try {
        db.prepareStatement(
            "verifyOrg",
            "SELECT id FROM organizations WHERE username = $1 AND password = $2;",
            2
        );
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;
    }

    try {
        db.prepareStatement("insertTicket",
            "INSERT INTO tickets (id, name, email, event_id, expiry) VALUES ($1, $2, $3, $4, $5);",
            5);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;
    }

    try {
        db.prepareStatement("scanTicket",
            "SELECT status from tickets WHERE id = $1",
            1);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;
    }

    try {
        db.prepareStatement("redeemTicket",
            "UPDATE tickets SET status = 'redeemed' WHERE id = $1",
            1);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;
    }

    try {
        db.prepareStatement(
            "getUserEvents",
            "SELECT id, name FROM events WHERE id = $1",
            1
        );
        cout << "Prepared statement successfully." << endl;
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
            string optionalDetails = (*json)["optional_details"].asString();

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
            auto resp = HttpResponse::newHttpResponse();

            try {

            
                string name = req->getParameter("name");
                string email = req->getParameter("email");
                string event_id = req->getParameter("event_id");
                

               
                
                


                if (name.empty() || email.empty() || event_id.empty()) {
                    resp->setStatusCode(k400BadRequest);
                    resp->setBody("Missing 'name' or 'id' or 'email' or 'event_id' or 'expiry'");
                    callback(resp);
                    return;
                }

                string id = generate_uuid();


               

               
                try {
                    db.execPrepared("insertTicket", { id, name, email, event_id });
                }
                catch (const runtime_error& e) {
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody(string("Event Creation failed: ") + e.what());
                    callback(resp);
                    return;
                }


                resp->setStatusCode(k200OK);
                resp->setBody("Ticket created successfully! \nTicket Id: " + id);
            }
            catch (const exception& e) {
              
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(string("Internal Server error: ") + e.what());
            }


            callback(resp);
        }
    );

    app().registerHandler(
        "/auth",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            
            auto json = req->getJsonObject();
            Json::Value res;

            if (!json || !(*json)["username"].isString() || !(*json)["password"].isString()) {
                res["success"] = false;
                res["error"] = "Invalid JSON";
                callback(HttpResponse::newHttpJsonResponse(res));
            }

            string username = (*json)["username"].asString();
            string password = (*json)["password"].asString();

            auto rows = db.execPrepared("verifyOrg", { username, password });

            if (rows.empty()) {
                res["success"] = false;
                res["error"] = "Incorrect credentials";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }

            string orgId = rows[0][0];

            res["success"] = true;
            res["id"] = orgId;

            callback(HttpResponse::newHttpJsonResponse(res));
        }
    );

    // ---- Events route ----
    app().registerHandler(
        "/mobile/events",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto json = req->getJsonObject();
            Json::Value res;

            if (!json || !(*json)["id"].isInt()) {
                res["success"] = false;
                res["error"] = "No user ID provided";
                callback(HttpResponse::newHttpJsonResponse(res));
                return;
            }
            
            string userId = (*json)["id"].asString();
            res["success"] = true;

            // Query events for this user
            auto rows = db.execPrepared("getUserEvents", { userId });

            Json::Value events(Json::arrayValue);
            for (auto& row : rows) {
                Json::Value e;
                e["id"] = std::stoi(row[0]);      // event id
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

            if (!json || !(*json)["eventId"].isInt() || !(*json)["ticketId"].isString()) {
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

            string ticketStatus = rows[0][0];

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

    /*drogon::app().registerHandler(
        "/create-ticket",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();

            try {
                QRPDFGenerator::generatePDF("abc", "adc", "ff", "hh");
            }
            catch (const exception& e) {
                cout << e.what();
            }


            callback(resp);
        }
    );*/

    drogon::app().addListener("0.0.0.0", myPort);
    drogon::app().run();

    return 0;
}
