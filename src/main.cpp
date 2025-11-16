#include <drogon/drogon.h>
#include <iostream>

int main() {
    using namespace drogon;

    int myPort = 8080;
    std::cout << "Server is starting on http://127.0.0.1:" << myPort << " ..." << std::endl;

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
