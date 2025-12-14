#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <stdexcept>

namespace Utils {

    // Converts a datetime string in "YYYY-MM-DD HH:MM:SS" format to `std::chrono::system_clock::time_point`
    inline std::chrono::system_clock::time_point parseDateTime(const std::string& datetime) {
        std::tm tm = {};
        std::istringstream ss(datetime);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) {
            throw std::runtime_error("Invalid date format: " + datetime);
        }

#if defined(_WIN32)
        std::time_t tt = _mkgmtime(&tm);
#else
        std::time_t tt = timegm(&tm);
#endif

        return std::chrono::system_clock::from_time_t(tt);
    }

    // Checks if an event can be created (dates are valid and in the future)
    inline void validateEventDates(const std::string& startsAt,
        const std::string& endsAt,
        const std::string& expiresAt) {
        auto startTime = parseDateTime(startsAt);
        auto endTime = parseDateTime(endsAt);
        auto expiryTime = parseDateTime(expiresAt);
        auto now = std::chrono::system_clock::now();

        if (startTime < now)
            throw std::runtime_error("Event start date/time has already passed");

        if (endTime < now)
            throw std::runtime_error("Event end date/time has already passed");

        if (endTime < startTime)
            throw std::runtime_error("Event end date/time cannot be before start date/time");

        if (expiryTime < endTime)
            throw std::runtime_error("Event expiry date/time must be after the event ends");
    }

    // Checks if a ticket can be created (event expiry is not passed)
    inline void validateTicketCreation(const std::string& eventExpiry) {
        auto expiryTime = parseDateTime(eventExpiry);
        auto now = std::chrono::system_clock::now();

        if (now > expiryTime)
            throw std::runtime_error("Event has expired, ticket cannot be created");
    }

} // namespace Utils
