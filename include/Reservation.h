#ifndef RESERVATION_H
#define RESERVATION_H

#include <string>
#include <vector>

enum class ReservationStatus {
    Confirmed,
    Pending,
    Cancelled
};

struct Reservation {
    int id;
    std::string customer;
    std::string service;
    std::string date;
    std::string time;
    int guests;
    ReservationStatus status;
};

const char* reservationStatusName(ReservationStatus status) noexcept;
bool parseReservationStatus(const std::string& value,
                            ReservationStatus& status) noexcept;
std::vector<Reservation> reservationDemoDataset();

#endif
