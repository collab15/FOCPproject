#include "QRPDFGenerator.hpp"
#include "qrcodegen.hpp"
#include <hpdf.h>
#include <stdexcept>

using namespace qrcodegen;
using std::string;
using std::runtime_error;


void QRPDFGenerator::generatePDF(const string& text, const string& filename) {
    // Generate QR code
    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::LOW);

    // Create PDF
    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) throw runtime_error("Failed to create PDF document");

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page, 300);
    HPDF_Page_SetHeight(page, 300);

    // QR code square size
    const float qrSize = 250.0f / qr.getSize();

    // Draw QR code
    for (int y = 0; y < qr.getSize(); ++y) {
        for (int x = 0; x < qr.getSize(); ++x) {
            if (qr.getModule(x, y)) {
                HPDF_Page_Rectangle(page, x * qrSize, (qr.getSize() - y - 1) * qrSize, qrSize, qrSize);
                HPDF_Page_Fill(page);
            }
        }
    }

    // Save PDF
    HPDF_SaveToFile(pdf, filename.c_str());
    HPDF_Free(pdf);
}
