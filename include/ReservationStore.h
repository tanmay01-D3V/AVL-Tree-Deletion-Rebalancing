#ifndef RESERVATION_STORE_H
#define RESERVATION_STORE_H

#include "AVLTree.h"
#include "Reservation.h"

#include <optional>
#include <string>
#include <vector>

struct sqlite3;

class ReservationStore {
public:
    explicit ReservationStore(const std::string& databasePath);
    ~ReservationStore();

    ReservationStore(const ReservationStore&) = delete;
    ReservationStore& operator=(const ReservationStore&) = delete;

    std::vector<Reservation> list(
        std::optional<ReservationStatus> status = std::nullopt) const;
    Reservation find(int id) const;
    void create(const Reservation& reservation);
    void createMany(const std::vector<Reservation>& reservations);
    void update(int oldId, const Reservation& reservation);
    void remove(int id);

    const AVLTree& index() const noexcept;
    std::vector<AVLRotationEvent> takeRotationEvents();
    bool verifySynchronization() const;

private:
    sqlite3* database_;
    AVLTree index_;
    std::vector<AVLRotationEvent> recentRotationEvents_;

    void initializeSchema();
    void rebuildIndex();
    std::optional<Reservation> findStored(int id) const;
    std::vector<Reservation> readAll() const;
    void recoverAfterFailure(const char* operation);

    static void validate(const Reservation& reservation);
};

#endif
