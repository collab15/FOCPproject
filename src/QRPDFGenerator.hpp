#pragma once
#include <string>

class QRPDFGenerator {
public:
    // Generates a PDF file with a QR code containing the given text
    // filename: path to save the PDF
    static void generatePDF(const std::string& id,
        const std::string& name,
        const std::string& venue,
        const std::string& optionalDetails,
        const std::string& startTimestamp,
        const std::string& endTimestamp,
        const std::string& filename);
};
