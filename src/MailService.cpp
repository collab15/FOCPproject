#include "MailService.h"
#include <curl/curl.h>

bool MailService::sendHtmlPdf(
    const std::string& to,
    const std::string& subject,
    const std::string& htmlBody,
    const std::string& pdfPath
) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* recipients = nullptr;
    struct curl_slist* headers = nullptr;
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part;

    curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp-relay.brevo.com:587");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    curl_easy_setopt(curl, CURLOPT_USERNAME,
        "9e90de001@smtp-brevo.com");

    // replace locally with your SMTP key
    curl_easy_setopt(curl, CURLOPT_PASSWORD,
        "REPLACE_WITH_YOUR_SMTP_KEY");

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM,
        "<noreply@qtick.app>");

    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    // HTML body
    part = curl_mime_addpart(mime);
    curl_mime_data(part, htmlBody.c_str(), CURL_ZERO_TERMINATED);
    curl_mime_type(part, "text/html; charset=UTF-8");

    // PDF attachment
    part = curl_mime_addpart(mime);
    curl_mime_filedata(part, pdfPath.c_str());
    curl_mime_filename(part, "ticket.pdf");
    curl_mime_type(part, "application/pdf");
    curl_mime_encoder(part, "base64");

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    std::string subjectHeader = "Subject: " + subject;
    headers = curl_slist_append(headers, subjectHeader.c_str());
    headers = curl_slist_append(headers, "From: QTick <noreply@qtick.app>");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(recipients);
    curl_slist_free_all(headers);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}
