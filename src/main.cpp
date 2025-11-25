#include <iostream>
#include <drogon/drogon.h>
#include "qrcodegen.hpp"

using namespace qrcodegen;

int main() {
    using namespace drogon;

    int myPort = 8080;
    std::cout << "\nServer is starting on http://127.0.0.1:" << myPort << " ..." << std::endl;

    const char* text = "Hello World";
    QrCode qr = QrCode::encodeText(text, QrCode::Ecc::MEDIUM);

	std::cout << "\n\nQR Code for text: " << text << "\n\n";
    // Print QR code in ASCII
    for (int y = 0; y < qr.getSize(); y++) {
        for (int x = 0; x < qr.getSize(); x++) {
            std::cout << (qr.getModule(x, y) ? "##" : "  ");
        }
        std::cout << "\n";
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
