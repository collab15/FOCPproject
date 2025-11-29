#include <iostream>
#include <fstream>
#include <drogon/drogon.h>
#include "qrcodegen.hpp"
#include "QRPDFGenerator.hpp"

// -----------------------------
// Namespaces
// -----------------------------
using namespace drogon;
using namespace qrcodegen;

using std::cout;
using std::cerr;
using std::endl;
using std::exception;
using std::function;

int main() {
    const int myPort = 8080;

    cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << endl;

    try {
        drogon::app().loadConfigFile("../../../config.json");
    }
    catch (const exception& e) {
        cerr << "loadConfigFile threw: " << e.what() << endl;
        return 1;
    }

    // Test the PostgreSQL connection
    auto db = drogon::app().getDbClient("pg");

    //try {
    //    cout << "\nGenerating sample QR-PDF 'ticket.pdf'..." << endl;
    //   QRPDFGenerator::generatePDF("Hello QR!", "ticket.pdf");
    //   cout << "Sample QR PDF 'ticket.pdf' generated successfully." << endl;
    //}
    //catch (const exception& e) {
    //    cerr << "Error: " << e.what() << endl;
    //}

    // Route: /test
    app().registerHandler(
        "/create-event",
        [db](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();

            db->execSqlAsync(
                "SELECT NOW()",
                [callback, resp](const drogon::orm::Result& r) {
                    resp->setBody("DB responded at: " + r[0]["now"].as<std::string>());
                    callback(resp);
                },
                [callback, resp](const drogon::orm::DrogonDbException& e) {
                    resp->setBody(std::string("Query failed: ") + e.base().what());
                    callback(resp);
                }
            );
        }
    );



    app().addListener("0.0.0.0", myPort);
    app().run();

    return 0;
}
