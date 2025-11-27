#include <iostream>
#include <drogon/drogon.h>
#include "qrcodegen.hpp"
#include "QRPDFGenerator.hpp"

using namespace qrcodegen;

int main() {
    using namespace drogon;

    int myPort = 8080;
    std::cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << std::endl;

    try {
		std::cout << "\nGenerating sample QR-PDF 'ticket.pdf'..." << std::endl;
        QRPDFGenerator::generatePDF("Hello QR!", "ticket.pdf");
		std::cout << "Sample QR PDF 'ticket.pdf' generated successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Minimal handler for "/test"
    app().registerHandler("/test",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setBody("Server works!");
            callback(resp);
        }
    );

    app().addListener("0.0.0.0", myPort); // Listen on all interfaces
    app().run();
}
