#include "QRPDFGenerator.hpp"
#include "qrcodegen.hpp"
#include <hpdf.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <ctime>
#include <iomanip>

std::string formatTimestamp(const std::string& ts) {
    // Expected: "2025-12-09 18:05:17+00"
    size_t pos = ts.find_first_of("+-", 11);
    std::string clean = (pos != std::string::npos ? ts.substr(0, pos) : ts);

    std::tm t = {};
    std::istringstream ss(clean);
    ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) return "";

    // Convert 24h -> 12h
    int hour = t.tm_hour;
    bool pm = hour >= 12;
    if (hour == 0) hour = 12;
    else if (hour > 12) hour -= 12;

    const char* months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };

    std::ostringstream out;
    out << t.tm_mday << " "
        << months[t.tm_mon] << ", "
        << (t.tm_year + 1900) << "  "
        << hour << ":"
        << std::setw(2) << std::setfill('0') << t.tm_min << " "
        << (pm ? "PM" : "AM") << " "
        << "UTC";

    return out.str();
}

using namespace qrcodegen;

void QRPDFGenerator::generatePDF(
    const std::string& id,
    const std::string& name,
    const std::string& venue,
    const std::string& optionalDetails,
    const std::string& startTimestamp,
    const std::string& endTimestamp,
    const std::string& filename)
{
    // Generate QR code
    QrCode qr = QrCode::encodeText(id.c_str(), QrCode::Ecc::LOW);

    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) throw std::runtime_error("Failed to create PDF document");

    HPDF_Page page = HPDF_AddPage(pdf);
    const float pageWidth = 300;
    const float pageHeight = 500;
    HPDF_Page_SetWidth(page, pageWidth);
    HPDF_Page_SetHeight(page, pageHeight);

    // Load background image
    HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, "bg.png");
    HPDF_Page_DrawImage(page, image, 0, 0, pageWidth, pageHeight);

    // Draw QR code
    const float qrDisplaySize = 180.0f;
    const float moduleSize = qrDisplaySize / qr.getSize();
    const float qrStartX = (pageWidth - qrDisplaySize) / 2;
    const float qrStartY = pageHeight - qrDisplaySize - 38;

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
                HPDF_Page_FillStroke(page);
            }
        }
    }

    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", nullptr);

    // Lambda: wrapped, centered text
    auto drawWrappedCenteredText = [&](float& y, const std::string& text, int fontSize) {
        HPDF_Page_SetFontAndSize(page, font, fontSize);
        const float maxWidth = pageWidth * 0.8f;
        const float lineHeight = fontSize + 2;
        std::istringstream iss(text);
        std::string word, line;

        HPDF_Page_BeginText(page);
        while (iss >> word) {
            std::string testLine = line.empty() ? word : line + " " + word;
            float testWidth = HPDF_Page_TextWidth(page, testLine.c_str());
            if (testWidth > maxWidth) {
                float lineWidth = HPDF_Page_TextWidth(page, line.c_str());
                HPDF_Page_TextOut(page, (pageWidth - lineWidth) / 2, y, line.c_str());
                y -= lineHeight;
                line = word;
            }
            else {
                line = testLine;
            }
        }
        if (!line.empty()) {
            float lineWidth = HPDF_Page_TextWidth(page, line.c_str());
            HPDF_Page_TextOut(page, (pageWidth - lineWidth) / 2, y, line.c_str());
            y -= lineHeight;
        }
        HPDF_Page_EndText(page);
        };

    // Lambda: single line centered text
    auto drawCenteredText = [&](float y, const std::string& text, int fontSize) {
        HPDF_Page_SetFontAndSize(page, font, fontSize);
        float width = HPDF_Page_TextWidth(page, text.c_str());
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, (pageWidth - width) / 2, y, text.c_str());
        HPDF_Page_EndText(page);
        };

    // Draw name and venue
    float currentY = qrStartY - 40;
    drawWrappedCenteredText(currentY, name, 18);   // title
    drawWrappedCenteredText(currentY, venue, 16);  // venue

    // Draw date/time range
    std::string startStr = formatTimestamp(startTimestamp);
    std::string endStr = formatTimestamp(endTimestamp);

    drawCenteredText(currentY - 10, startStr, 15);
    drawCenteredText(currentY - 28, "to", 15);
    drawCenteredText(currentY - 46, endStr, 15);
    currentY -= 70;

    // Draw optional details
    drawWrappedCenteredText(currentY, optionalDetails, 12);

    // Save PDF
    HPDF_STATUS status = HPDF_SaveToFile(pdf, filename.c_str());
    if (status != HPDF_OK) {
        HPDF_Free(pdf);
        throw std::runtime_error("Failed to save PDF");
    }

    HPDF_Free(pdf);
}
