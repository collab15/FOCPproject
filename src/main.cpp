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

int main() {
    const int myPort = 8080;
    cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << endl;

    // Initialize DB (throws runtime_error on connection failure)

    PostgresDB db("postgresql://postgres:NoorHuda123@aws-1-ap-south-1.pooler.supabase.com:5432/postgres");


    // Prepare insert statement once (throws on failure)
    try {
        db.prepareStatement("insertEvent", "INSERT INTO events (id, name, venue) VALUES ($1, $2, $3);", 3);
        cout << "Prepared statement successfully." << endl;
    }
    catch (const runtime_error& e) {
        cerr << "Failed to prepare statement: " << e.what() << endl;
        return 1;  // Stop program
    }

    // Route: /create-event
    app().registerHandler(
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

                string eventId = generate_uuid();
                cout << "Generated UUID: " << eventId << endl;

                string name = "Event Name";
				string venue = "Event Venue";

                // Insert into DB (throws on failure)
                try {
                    db.execPrepared("insertEvent", { eventId, name, venue });
                }
                catch (const runtime_error& e) {
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody(string("Event Creation failed: ") + e.what());
                    callback(resp);
                }

                // Generate QR-PDF (throws on failure)
                try {
                    QRPDFGenerator::generatePDF(eventId, "ticket.pdf");
                }
                catch (const runtime_error& e) {
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody(string("QR-PDF generation failed: ") + e.what());
                    callback(resp);
                }

                // Success
                resp->setStatusCode(k200OK);
                resp->setBody("Event created successfully! QR-PDF generated: ");
            }
            catch (const std::exception& e) {
                // Catch any unexpected exception
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(string("Internal Server error: ") + e.what());
            }

            callback(resp);
        }
    );

    app().addListener("0.0.0.0", myPort);
    app().run();

    return 0;
}
