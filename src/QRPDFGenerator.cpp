#include "QRPDFGenerator.hpp"
#include "qrcodegen.hpp"
#include <hpdf.h>
#include <stdexcept>

using namespace qrcodegen;

void QRPDFGenerator::generatePDF(
    const std::string& id,
    const std::string& name,
    const std::string& venue,
    const std::string& filename) {
    // Generate QR code (from id only)
    QrCode qr = QrCode::encodeText(id.c_str(), QrCode::Ecc::LOW);

    // Create PDF
    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) throw std::runtime_error("Failed to create PDF document");

    HPDF_Page page = HPDF_AddPage(pdf);

    const float pageWidth = 300;
    const float pageHeight = 500;  // Increased to fit text

    HPDF_Page_SetWidth(page, pageWidth);
    HPDF_Page_SetHeight(page, pageHeight);

    // ------ QR CODE POSITIONING ------
    const float qrDisplaySize = 180.0f;                   // Physical displayed size
    const float moduleSize = qrDisplaySize / qr.getSize();
    const float qrStartX = (pageWidth - qrDisplaySize) / 2;   // Center horizontally
    const float qrStartY = pageHeight - qrDisplaySize - 20;   // Margin from top

    // Draw QR code
    for (int y = 0; y < qr.getSize(); ++y) {
        for (int x = 0; x < qr.getSize(); ++x) {
            if (qr.getModule(x, y)) {
                HPDF_Page_Rectangle(
                    page,
                    qrStartX + x * moduleSize,
                    qrStartY + (qr.getSize() - y - 1) * moduleSize,
                    moduleSize,
                    moduleSize
                );
                HPDF_Page_Fill(page);
            }
        }
    }

    // ------ TEXT ------
    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", nullptr);
    HPDF_Page_SetFontAndSize(page, font, 14);

    // NAME (centered)
    float nameWidth = HPDF_Page_TextWidth(page, name.c_str());
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page,
        (pageWidth - nameWidth) / 2,
        qrStartY - 30,
        name.c_str());
    HPDF_Page_EndText(page);

    // VENUE (centered)
    float venueWidth = HPDF_Page_TextWidth(page, venue.c_str());
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page,
        (pageWidth - venueWidth) / 2,
        qrStartY - 55,
        venue.c_str());
    HPDF_Page_EndText(page);

    // Save & free resources
    HPDF_SaveToFile(pdf, filename.c_str());
    HPDF_Free(pdf);
}
