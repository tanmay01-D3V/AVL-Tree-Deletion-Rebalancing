#include "ReservationStore.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

class Statement {
public:
    Statement(sqlite3* database, const char* sql) : database_(database) {
        const int result = sqlite3_prepare_v2(database_, sql, -1, &statement_, nullptr);
        if (result != SQLITE_OK) {
            throw std::runtime_error(std::string("Preparing SQLite statement failed: ")
                                     + sqlite3_errmsg(database_));
        }
    }

    ~Statement() {
        sqlite3_finalize(statement_);
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void bindInt(int index, int value) {
        check(sqlite3_bind_int(statement_, index, value), "Binding integer failed");
    }

    void bindText(int index, const std::string& value) {
        check(sqlite3_bind_text(statement_, index, value.c_str(),
                                static_cast<int>(value.size()), SQLITE_TRANSIENT),
              "Binding text failed");
    }

    bool next() {
        const int result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) {
            return true;
        }
        if (result == SQLITE_DONE) {
            return false;
        }
        check(result, "Executing SQLite statement failed");
        return false;
    }

    void execute() {
        if (next()) {
            throw std::runtime_error("SQLite statement unexpectedly returned a row");
        }
    }

    long long int64Column(int index) const {
        return sqlite3_column_int64(statement_, index);
    }

    std::string textColumn(int index) const {
        const auto* value = sqlite3_column_text(statement_, index);
        if (value == nullptr) {
            return {};
        }
        const int length = sqlite3_column_bytes(statement_, index);
        return std::string(reinterpret_cast<const char*>(value),
                           static_cast<std::size_t>(length));
    }

private:
    sqlite3* database_;
    sqlite3_stmt* statement_ = nullptr;

    void check(int result, const char* operation) const {
        if (result != SQLITE_OK && result != SQLITE_DONE) {
            throw std::runtime_error(std::string(operation) + ": "
                                     + sqlite3_errmsg(database_));
        }
    }
};

class Transaction {
public:
    explicit Transaction(sqlite3* database) : database_(database) {
        executeSql(database_, "BEGIN IMMEDIATE");
    }

    ~Transaction() {
        if (active_) {
            sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit() {
        executeSql(database_, "COMMIT");
        active_ = false;
    }

    void rollback() {
        if (active_) {
            executeSql(database_, "ROLLBACK");
            active_ = false;
        }
    }

private:
    sqlite3* database_;
    bool active_ = true;

    static void executeSql(sqlite3* database, const char* sql) {
        char* error = nullptr;
        const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error);
        if (result != SQLITE_OK) {
            const std::string message = error == nullptr
                ? sqlite3_errmsg(database)
                : error;
            sqlite3_free(error);
            throw std::runtime_error(std::string("SQLite transaction failed: ") + message);
        }
    }
};

bool isBlank(const std::string& value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isspace(character) != 0;
    });
}

bool hasControlCharacters(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20 || character == 0x7f;
    });
}

bool hasDigits(const std::string& value, std::size_t begin, std::size_t end) {
    return std::all_of(value.begin() + static_cast<std::ptrdiff_t>(begin),
                       value.begin() + static_cast<std::ptrdiff_t>(end),
                       [](unsigned char character) {
                           return std::isdigit(character) != 0;
                       });
}

int parseDigits(const std::string& value, std::size_t begin, std::size_t length) {
    return std::stoi(value.substr(begin, length));
}

bool isValidDate(const std::string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-'
        || !hasDigits(value, 0, 4) || !hasDigits(value, 5, 7)
        || !hasDigits(value, 8, 10)) {
        return false;
    }

    const int year = parseDigits(value, 0, 4);
    const int month = parseDigits(value, 5, 2);
    const int day = parseDigits(value, 8, 2);
    static constexpr int daysPerMonth[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (year == 0 || month < 1 || month > 12) {
        return false;
    }

    int maximumDay = daysPerMonth[month - 1];
    const bool leapYear = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    if (month == 2 && leapYear) {
        ++maximumDay;
    }
    return day >= 1 && day <= maximumDay;
}

bool isValidTime(const std::string& value) {
    if (value.size() != 5 || value[2] != ':'
        || !hasDigits(value, 0, 2) || !hasDigits(value, 3, 5)) {
        return false;
    }

    const int hour = parseDigits(value, 0, 2);
    const int minute = parseDigits(value, 3, 2);
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

Reservation readReservation(Statement& statement) {
    const long long id = statement.int64Column(0);
    const long long guests = statement.int64Column(5);
    if (id < std::numeric_limits<int>::min()
        || id > std::numeric_limits<int>::max()
        || guests < std::numeric_limits<int>::min()
        || guests > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Database contains a reservation value outside the integer range");
    }

    Reservation reservation{
        static_cast<int>(id),
        statement.textColumn(1),
        statement.textColumn(2),
        statement.textColumn(3),
        statement.textColumn(4),
        static_cast<int>(guests),
        ReservationStatus::Pending
    };

    if (!parseReservationStatus(statement.textColumn(6), reservation.status)) {
        throw std::runtime_error("Database contains an invalid reservation status");
    }
    return reservation;
}

bool validateSnapshot(const std::vector<AVLTreeNodeSnapshot>& nodes) {
    if (nodes.empty()) {
        return true;
    }

    std::vector<bool> visited(nodes.size(), false);
    const auto visit = [&](const auto& self, int index, long long lower,
                           long long upper, int& height) -> bool {
        if (index == -1) {
            height = 0;
            return true;
        }
        if (index < 0 || static_cast<std::size_t>(index) >= nodes.size()
            || visited[static_cast<std::size_t>(index)]) {
            return false;
        }

        visited[static_cast<std::size_t>(index)] = true;
        const AVLTreeNodeSnapshot& node = nodes[static_cast<std::size_t>(index)];
        if (node.key <= lower || node.key >= upper) {
            return false;
        }

        int leftHeight = 0;
        int rightHeight = 0;
        if (!self(self, node.leftIndex, lower, node.key, leftHeight)
            || !self(self, node.rightIndex, node.key, upper, rightHeight)) {
            return false;
        }

        height = 1 + std::max(leftHeight, rightHeight);
        return node.height == height
            && node.balanceFactor == leftHeight - rightHeight
            && std::abs(node.balanceFactor) <= 1;
    };

    int height = 0;
    if (!visit(visit, 0, std::numeric_limits<long long>::min(),
               std::numeric_limits<long long>::max(), height)) {
        return false;
    }
    return std::all_of(visited.begin(), visited.end(), [](bool value) {
        return value;
    });
}

} // namespace

const char* reservationStatusName(ReservationStatus status) noexcept {
    switch (status) {
    case ReservationStatus::Confirmed:
        return "Confirmed";
    case ReservationStatus::Pending:
        return "Pending";
    case ReservationStatus::Cancelled:
        return "Cancelled";
    }
    return "Pending";
}

bool parseReservationStatus(const std::string& value,
                            ReservationStatus& status) noexcept {
    if (value == "Confirmed") {
        status = ReservationStatus::Confirmed;
        return true;
    }
    if (value == "Pending") {
        status = ReservationStatus::Pending;
        return true;
    }
    if (value == "Cancelled") {
        status = ReservationStatus::Cancelled;
        return true;
    }
    return false;
}

ReservationStore::ReservationStore(const std::string& databasePath)
    : database_(nullptr) {
    if (databasePath != ":memory:") {
        const std::filesystem::path path(databasePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    }

    const int result = sqlite3_open_v2(
        databasePath.c_str(), &database_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr);
    if (result != SQLITE_OK) {
        const std::string message = database_ == nullptr
            ? "Unable to open SQLite database"
            : sqlite3_errmsg(database_);
        if (database_ != nullptr) {
            sqlite3_close(database_);
            database_ = nullptr;
        }
        throw std::runtime_error("Opening reservation database failed: " + message);
    }

    sqlite3_busy_timeout(database_, 5000);
    initializeSchema();
    rebuildIndex();
}

ReservationStore::~ReservationStore() {
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

std::vector<Reservation> ReservationStore::list(
    std::optional<ReservationStatus> status) const {
    const char* allSql =
        "SELECT id, customer, service, reservation_date, reservation_time, guests, status "
        "FROM reservations ORDER BY id";
    const char* filteredSql =
        "SELECT id, customer, service, reservation_date, reservation_time, guests, status "
        "FROM reservations WHERE status = ? ORDER BY id";
    Statement statement(database_, status.has_value() ? filteredSql : allSql);
    if (status.has_value()) {
        statement.bindText(1, reservationStatusName(*status));
    }

    std::vector<Reservation> reservations;
    while (statement.next()) {
        reservations.push_back(readReservation(statement));
    }
    return reservations;
}

Reservation ReservationStore::find(int id) const {
    const bool indexed = index_.search(id);
    const std::optional<Reservation> reservation = findStored(id);
    if (indexed != reservation.has_value()) {
        throw std::runtime_error("Reservation database and AVL index are out of sync");
    }
    if (!reservation.has_value()) {
        throw std::out_of_range("Reservation not found");
    }
    return *reservation;
}

void ReservationStore::create(const Reservation& reservation) {
    createMany({reservation});
}

void ReservationStore::createMany(const std::vector<Reservation>& reservations) {
    recentRotationEvents_.clear();
    std::vector<int> ids;
    ids.reserve(reservations.size());
    for (const Reservation& reservation : reservations) {
        validate(reservation);
        const bool indexed = index_.search(reservation.id);
        const bool stored = findStored(reservation.id).has_value();
        if (indexed != stored) {
            throw std::runtime_error("Reservation database and AVL index are out of sync");
        }
        if (indexed
            || std::find(ids.begin(), ids.end(), reservation.id) != ids.end()) {
            throw std::invalid_argument("A reservation ID already exists");
        }
        ids.push_back(reservation.id);
    }

    Transaction transaction(database_);
    try {
        for (const Reservation& reservation : reservations) {
            Statement statement(
                database_,
                "INSERT INTO reservations "
                "(id, customer, service, reservation_date, reservation_time, guests, status) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)");
            statement.bindInt(1, reservation.id);
            statement.bindText(2, reservation.customer);
            statement.bindText(3, reservation.service);
            statement.bindText(4, reservation.date);
            statement.bindText(5, reservation.time);
            statement.bindInt(6, reservation.guests);
            statement.bindText(7, reservationStatusName(reservation.status));
            statement.execute();

            index_.insert(reservation.id);
            std::vector<AVLRotationEvent> insertionEvents = index_.takeRotationEvents();
            recentRotationEvents_.insert(recentRotationEvents_.end(),
                                         insertionEvents.begin(),
                                         insertionEvents.end());
        }
        if (!verifySynchronization()) {
            throw std::runtime_error("Reservation insert did not preserve AVL invariants");
        }
        transaction.commit();
    } catch (...) {
        transaction.rollback();
        recoverAfterFailure("create");
        throw;
    }
}

void ReservationStore::update(int oldId, const Reservation& reservation) {
    recentRotationEvents_.clear();
    validate(reservation);
    find(oldId);
    if (reservation.id != oldId
        && (index_.search(reservation.id) || findStored(reservation.id).has_value())) {
        throw std::invalid_argument("A reservation with the new ID already exists");
    }

    Transaction transaction(database_);
    try {
        Statement statement(
            database_,
            "UPDATE reservations SET id = ?, customer = ?, service = ?, "
            "reservation_date = ?, reservation_time = ?, guests = ?, status = ? "
            "WHERE id = ?");
        statement.bindInt(1, reservation.id);
        statement.bindText(2, reservation.customer);
        statement.bindText(3, reservation.service);
        statement.bindText(4, reservation.date);
        statement.bindText(5, reservation.time);
        statement.bindInt(6, reservation.guests);
        statement.bindText(7, reservationStatusName(reservation.status));
        statement.bindInt(8, oldId);
        statement.execute();
        if (sqlite3_changes(database_) != 1) {
            throw std::runtime_error("Reservation update affected an unexpected number of rows");
        }

        if (reservation.id != oldId) {
            index_.insert(reservation.id);
            std::vector<AVLRotationEvent> insertionEvents = index_.takeRotationEvents();
            recentRotationEvents_.insert(recentRotationEvents_.end(),
                                         insertionEvents.begin(),
                                         insertionEvents.end());
            if (!index_.remove(oldId)) {
                throw std::runtime_error("Old reservation ID was missing from the AVL index");
            }
            std::vector<AVLRotationEvent> deletionEvents = index_.takeRotationEvents();
            recentRotationEvents_.insert(recentRotationEvents_.end(),
                                         deletionEvents.begin(),
                                         deletionEvents.end());
        }

        if (!verifySynchronization()) {
            throw std::runtime_error("Reservation update did not preserve AVL invariants");
        }
        transaction.commit();
    } catch (...) {
        transaction.rollback();
        recoverAfterFailure("update");
        throw;
    }
}

void ReservationStore::remove(int id) {
    recentRotationEvents_.clear();
    find(id);
    Transaction transaction(database_);
    try {
        Statement statement(database_, "DELETE FROM reservations WHERE id = ?");
        statement.bindInt(1, id);
        statement.execute();
        if (sqlite3_changes(database_) != 1) {
            throw std::runtime_error("Reservation delete affected an unexpected number of rows");
        }
        if (!index_.remove(id)) {
            throw std::runtime_error("Reservation ID was missing from the AVL index");
        }
        recentRotationEvents_ = index_.takeRotationEvents();

        if (!verifySynchronization()) {
            throw std::runtime_error("Reservation delete did not preserve AVL invariants");
        }
        transaction.commit();
    } catch (...) {
        transaction.rollback();
        recoverAfterFailure("delete");
        throw;
    }
}

const AVLTree& ReservationStore::index() const noexcept {
    return index_;
}

std::vector<AVLRotationEvent> ReservationStore::takeRotationEvents() {
    std::vector<AVLRotationEvent> events;
    events.swap(recentRotationEvents_);
    return events;
}

bool ReservationStore::verifySynchronization() const {
    const std::vector<Reservation> reservations = readAll();
    const std::vector<AVLTreeNodeSnapshot> nodes = index_.snapshot();
    if (!validateSnapshot(nodes) || nodes.size() != reservations.size()) {
        return false;
    }

    std::vector<int> storedIds;
    storedIds.reserve(reservations.size());
    for (const Reservation& reservation : reservations) {
        storedIds.push_back(reservation.id);
    }
    return index_.inorderTraversal() == storedIds;
}

void ReservationStore::initializeSchema() {
    char* error = nullptr;
    const int result = sqlite3_exec(
        database_,
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS reservations ("
        "id INTEGER PRIMARY KEY CHECK (id > 0),"
        "customer TEXT NOT NULL,"
        "service TEXT NOT NULL,"
        "reservation_date TEXT NOT NULL,"
        "reservation_time TEXT NOT NULL,"
        "guests INTEGER NOT NULL CHECK (guests > 0),"
        "status TEXT NOT NULL CHECK (status IN ('Confirmed', 'Pending', 'Cancelled'))"
        ");",
        nullptr, nullptr, &error);
    if (result != SQLITE_OK) {
        const std::string message = error == nullptr
            ? sqlite3_errmsg(database_)
            : error;
        sqlite3_free(error);
        throw std::runtime_error("Initializing reservation database failed: " + message);
    }
}

void ReservationStore::rebuildIndex() {
    index_.clear();
    recentRotationEvents_.clear();
    for (const Reservation& reservation : readAll()) {
        validate(reservation);
        if (index_.search(reservation.id)) {
            throw std::runtime_error("Database contains a duplicate reservation ID");
        }
        index_.insert(reservation.id);
        index_.takeRotationEvents();
    }
    if (!verifySynchronization()) {
        throw std::runtime_error("Rebuilt reservation index failed its invariant check");
    }
}

std::optional<Reservation> ReservationStore::findStored(int id) const {
    Statement statement(
        database_,
        "SELECT id, customer, service, reservation_date, reservation_time, guests, status "
        "FROM reservations WHERE id = ?");
    statement.bindInt(1, id);
    if (!statement.next()) {
        return std::nullopt;
    }
    return readReservation(statement);
}

std::vector<Reservation> ReservationStore::readAll() const {
    Statement statement(
        database_,
        "SELECT id, customer, service, reservation_date, reservation_time, guests, status "
        "FROM reservations ORDER BY id");
    std::vector<Reservation> reservations;
    while (statement.next()) {
        reservations.push_back(readReservation(statement));
    }
    return reservations;
}

void ReservationStore::recoverAfterFailure(const char* operation) {
    try {
        rebuildIndex();
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("Reservation ") + operation
                                 + " failed and AVL index recovery failed: "
                                 + error.what());
    }
}

void ReservationStore::validate(const Reservation& reservation) {
    if (reservation.id <= 0) {
        throw std::invalid_argument("Reservation ID must be a positive integer");
    }
    if (reservation.customer.empty() || isBlank(reservation.customer)
        || hasControlCharacters(reservation.customer)
        || reservation.customer.size() > 100) {
        throw std::invalid_argument("Customer name must be non-empty and at most 100 bytes");
    }
    if (reservation.service.empty() || isBlank(reservation.service)
        || hasControlCharacters(reservation.service)
        || reservation.service.size() > 100) {
        throw std::invalid_argument("Service must be non-empty and at most 100 bytes");
    }
    if (!isValidDate(reservation.date)) {
        throw std::invalid_argument("Reservation date must be a valid YYYY-MM-DD date");
    }
    if (!isValidTime(reservation.time)) {
        throw std::invalid_argument("Reservation time must be a valid 24-hour HH:MM time");
    }
    if (reservation.guests < 1 || reservation.guests > 1000) {
        throw std::invalid_argument("Guests/resources must be between 1 and 1000");
    }
    if (reservation.status != ReservationStatus::Confirmed
        && reservation.status != ReservationStatus::Pending
        && reservation.status != ReservationStatus::Cancelled) {
        throw std::invalid_argument("Reservation status is invalid");
    }
}
