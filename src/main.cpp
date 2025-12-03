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

int main() {
    const int myPort = 8080;
    cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << endl;

    // Initialize DB (throws runtime_error on connection failure)

    PostgresDB db("postgresql://postgres.izjgblghemuroxvptbql:NoorHuda123@aws-1-ap-south-1.pooler.supabase.com:5432/postgres");


    // Prepare insert statement once (throws on failure)
    try {
        db.prepareStatement("insertEvent", "INSERT INTO events (event_id, name, venue, created_at,date, time, optional_details, organization_id) VALUES ($1, $2, $3 ,$4,$5,$6,$7,$8);", 3);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;  // Stop program
    }

    // Route: /create-event
    drogon::app().registerHandler(
        "/create-event",
        [&db](const HttpRequestPtr& req, function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();

            try {
                // Extract POST parameters
                //auto json = req->getJsonObject();
                //if (!json || !(*json)["name"].isString() || !(*json)["venue"].isString()) {
                //    resp->setStatusCode(k400BadRequest);
                //    resp->setBody("Missing or invalid 'name' or 'venue'");
                //    callback(resp);
                //    return;
                //}

                //string name = (*json)["name"].asString();
                //string venue = (*json)["venue"].asString();

               
                string venue = req->getParameter("venue");
                string time = req->getParameter("time");
                string date = req->getParameter("date");
                string optional_details = req->getParameter("optional_details");
                string username = req->getParameter("username");
                string password = req->getParameter("password");

                db.prepareStatement(
                    "getOrg",
                    "SELECT organization_id FROM organizations WHERE username = $1 AND password = $2;",
                    2
                );

                auto rows = db.execPrepared("getOrg", { username, password });

                if (rows.empty()) {
                    resp->setStatusCode(k401Unauthorized);
                    resp->setBody("Invalid organization name or password");
                    return;
                }

                std::string orgId = rows[0][0];

          

                
               

                if (username.empty() || venue.empty() || time.empty() || date.empty() || optional_details.empty()) {
                    resp->setStatusCode(k400BadRequest);
                    resp->setBody("Missing detail'");
                    callback(resp);
                    return;

                }


                string eventId = generate_uuid();

                // Insert into DB (throws on failure)
                try {
                    db.execPrepared("insertEvent", { eventId, username, venue, time, date, optional_details, orgId });
                }
                catch (const runtime_error& e) {
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody(string("Event Creation failed: ") + e.what());
                    callback(resp);
                    return;
                }


                // Success
                resp->setStatusCode(k200OK);
                resp->setBody("Event created successfully! \nEventId: " + eventId);
            }
            catch (const exception& e) {
                // Catch any unexpected exception
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(string("Internal Server error: ") + e.what());
            }

            callback(resp);
        }
    );

    
    try {
        db.prepareStatement("insertTicket", "INSERT INTO tickets (id, name, email, event_id, expiry) VALUES ($1, $2, $3, $4, $5);", 5);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1; 
    }

    
    drogon::app().registerHandler(
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

    drogon::app().registerHandler(
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
    );

    drogon::app().addListener("0.0.0.0", myPort);
    drogon::app().run();

    return 0;
}
