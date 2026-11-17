#include "Reservation.h"

#include <array>

std::vector<Reservation> reservationDemoDataset() {
    const std::array<int, 12> ids = {
        1010, 1003, 1008, 1005, 1012, 1006, 1002, 1007, 1004, 1011, 1001, 1009
    };
    const std::array<const char*, 12> customers = {
        "Mara Chen", "Noah Rivera", "Iris Morgan", "Leo Bennett",
        "Ava Patel", "Eli Foster", "Nia Brooks", "Owen Price",
        "Mina Park", "Theo James", "Zoe Lin", "Sam Ellis"
    };
    const std::array<const char*, 12> services = {
        "Garden table", "Studio session", "Guided tour", "Workshop seat",
        "Meeting room", "Garden table", "Studio session", "Guided tour",
        "Workshop seat", "Meeting room", "Garden table", "Studio session"
    };
    std::vector<Reservation> result;
    result.reserve(ids.size());
    for (std::size_t index = 0; index < ids.size(); ++index) {
        result.push_back({
            ids[index],
            customers[index],
            services[index],
            "2026-12-01",
            (index % 2 == 0) ? "10:30" : "14:00",
            static_cast<int>(index % 4) + 1,
            index % 3 == 0 ? ReservationStatus::Confirmed
                : (index % 3 == 1 ? ReservationStatus::Pending
                                  : ReservationStatus::Cancelled)
        });
    }
    return result;
}
