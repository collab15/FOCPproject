#pragma once
#include <string>

class MailService {
public:
    static bool sendHtmlPdf(
        const std::string& to,
        const std::string& subject,
        const std::string& htmlBody,
        const std::string& pdfPath
    );
};
