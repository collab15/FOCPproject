#pragma once
#include <string>

class QRPDFGenerator {
public:
    // Generates a PDF file with a QR code containing the given text
    // filename: path to save the PDF
    static void generatePDF(const std::string& text, const std::string& filename);
};
