#include "AVLTree.h"
#include "Reservation.h"
#include "ReservationStore.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void assertTreeInvariants(const AVLTree& tree) {
    const std::vector<AVLTreeNodeSnapshot> nodes = tree.snapshot();
    if (nodes.empty()) {
        assert(tree.getHeight() == 0);
        assert(tree.inorderTraversal().empty());
        return;
    }

    std::vector<bool> visited(nodes.size(), false);
    const auto visit = [&](const auto& self, int index, long long lower,
                           long long upper) -> int {
        if (index == -1) {
            return 0;
        }
        assert(index >= 0);
        assert(static_cast<std::size_t>(index) < nodes.size());
        assert(!visited[static_cast<std::size_t>(index)]);
        visited[static_cast<std::size_t>(index)] = true;

        const AVLTreeNodeSnapshot& node = nodes[static_cast<std::size_t>(index)];
        assert(node.key > lower && node.key < upper);
        const int leftHeight = self(self, node.leftIndex, lower, node.key);
        const int rightHeight = self(self, node.rightIndex, node.key, upper);
        const int expectedHeight = 1 + std::max(leftHeight, rightHeight);
        assert(node.height == expectedHeight);
        assert(node.balanceFactor == leftHeight - rightHeight);
        assert(-1 <= node.balanceFactor && node.balanceFactor <= 1);
        return expectedHeight;
    };

    assert(visit(visit, 0, std::numeric_limits<long long>::min(),
                 std::numeric_limits<long long>::max()) == tree.getHeight());
    assert(std::all_of(visited.begin(), visited.end(), [](bool value) {
        return value;
    }));
}

int childCount(const AVLTree& tree, int key) {
    for (const AVLTreeNodeSnapshot& node : tree.snapshot()) {
        if (node.key == key) {
            return (node.leftIndex != -1 ? 1 : 0)
                + (node.rightIndex != -1 ? 1 : 0);
        }
    }
    return -1;
}

Reservation makeReservation(int id,
                            ReservationStatus status = ReservationStatus::Pending) {
    return {id, "Casey Example", "Studio session", "2026-12-01",
            "10:30", 2, status};
}

void testRandomizedOperationsAgainstSet() {
    AVLTree tree;
    std::set<int> expected;
    std::mt19937 generator(0x5A17U);
    std::uniform_int_distribution<int> keyDistribution(-250, 250);
    std::bernoulli_distribution insertDistribution(0.55);

    for (int operation = 0; operation < 3000; ++operation) {
        const int key = keyDistribution(generator);
        if (insertDistribution(generator)) {
            tree.insert(key);
            expected.insert(key);
        } else {
            const bool removed = tree.remove(key);
            const bool expectedRemoved = expected.erase(key) != 0;
            assert(removed == expectedRemoved);
        }
        tree.takeRotationEvents();

        const std::vector<int> expectedValues(expected.begin(), expected.end());
        assert(tree.inorderTraversal() == expectedValues);
        assertTreeInvariants(tree);
    }
}

void testReservationCrudAndIndexSynchronization() {
    ReservationStore store(":memory:");
    assert(store.verifySynchronization());

    store.create(makeReservation(20, ReservationStatus::Confirmed));
    assert(store.index().search(20));
    assert(store.find(20).customer == "Casey Example");
    assert(store.verifySynchronization());

    bool duplicateRejected = false;
    try {
        store.create(makeReservation(20));
    } catch (const std::invalid_argument&) {
        duplicateRejected = true;
    }
    assert(duplicateRejected);
    assert(store.verifySynchronization());

    store.create(makeReservation(30, ReservationStatus::Cancelled));
    Reservation updated = makeReservation(25, ReservationStatus::Pending);
    updated.customer = "Updated Customer";
    store.update(20, updated);
    assert(!store.index().search(20));
    assert(store.index().search(25));
    assert(store.find(25).customer == "Updated Customer");
    assert(store.verifySynchronization());

    bool duplicateUpdateRejected = false;
    try {
        store.update(25, makeReservation(30));
    } catch (const std::invalid_argument&) {
        duplicateUpdateRejected = true;
    }
    assert(duplicateUpdateRejected);
    assert(store.index().search(25));
    assert(store.index().search(30));
    assert(store.verifySynchronization());

    assert(store.list(ReservationStatus::Cancelled).size() == 1);
    store.remove(25);
    assert(!store.index().search(25));
    assert(store.verifySynchronization());

    bool missingSearchReported = false;
    try {
        store.find(999);
    } catch (const std::out_of_range&) {
        missingSearchReported = true;
    }
    assert(missingSearchReported);

    bool missingDeleteReported = false;
    try {
        store.remove(999);
    } catch (const std::out_of_range&) {
        missingDeleteReported = true;
    }
    assert(missingDeleteReported);
    assert(store.verifySynchronization());
}

void testInvalidReservationFieldsAreRejected() {
    ReservationStore store(":memory:");
    Reservation invalid = makeReservation(1);
    invalid.date = "2026-02-29";
    bool invalidDateRejected = false;
    try {
        store.create(invalid);
    } catch (const std::invalid_argument&) {
        invalidDateRejected = true;
    }
    assert(invalidDateRejected);

    invalid = makeReservation(1);
    invalid.time = "24:00";
    bool invalidTimeRejected = false;
    try {
        store.create(invalid);
    } catch (const std::invalid_argument&) {
        invalidTimeRejected = true;
    }
    assert(invalidTimeRejected);

    invalid = makeReservation(1);
    invalid.customer = std::string("Bad\0name", 8);
    bool invalidTextRejected = false;
    try {
        store.create(invalid);
    } catch (const std::invalid_argument&) {
        invalidTextRejected = true;
    }
    assert(invalidTextRejected);
    assert(store.list().empty());
    assert(store.verifySynchronization());
}

void testDemoExercisesRotationsAndDeletionCases() {
    ReservationStore store(":memory:");
    const std::vector<Reservation> sample = reservationDemoDataset();
    store.createMany(sample);
    assert(store.verifySynchronization());

    std::array<bool, 4> insertionRotations{};
    for (const AVLRotationEvent& event : store.takeRotationEvents()) {
        assert(event.mutation == AVLMutationType::Insert);
        insertionRotations[static_cast<std::size_t>(event.type)] = true;
    }
    assert(std::all_of(insertionRotations.begin(), insertionRotations.end(),
                       [](bool value) { return value; }));

    assert(store.find(1011).id == 1011);
    assert(childCount(store.index(), 1004) == 0);
    store.remove(1004);
    const std::vector<AVLRotationEvent> multiAncestorRotations =
        store.takeRotationEvents();
    assert(multiAncestorRotations.size() == 2);
    assert(multiAncestorRotations[0].mutation == AVLMutationType::Delete);
    assert(multiAncestorRotations[0].operationKey == 1004);
    assert(multiAncestorRotations[1].mutation == AVLMutationType::Delete);
    assert(multiAncestorRotations[1].operationKey == 1004);
    assert(store.verifySynchronization());

    assert(childCount(store.index(), 1006) == 1);
    store.remove(1006);
    assert(store.verifySynchronization());

    assert(childCount(store.index(), 1008) == 2);
    store.remove(1008);
    assert(store.verifySynchronization());
    assert(store.list().size() == sample.size() - 3);
}

void testReservationPersistenceRebuildsTheIndex() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path databasePath =
        std::filesystem::temp_directory_path()
        / ("avl-studio-reservation-test-" + std::to_string(stamp) + ".sqlite3");
    const std::string path = databasePath.string();

    {
        ReservationStore store(path);
        store.create(makeReservation(73, ReservationStatus::Confirmed));
        assert(store.verifySynchronization());
    }
    {
        ReservationStore reopened(path);
        assert(reopened.index().search(73));
        assert(reopened.find(73).status == ReservationStatus::Confirmed);
        assert(reopened.verifySynchronization());
    }

    std::filesystem::remove(databasePath);
    std::filesystem::remove(path + "-wal");
    std::filesystem::remove(path + "-shm");
}

} // namespace

int main() {
    testRandomizedOperationsAgainstSet();
    testReservationCrudAndIndexSynchronization();
    testInvalidReservationFieldsAreRejected();
    testDemoExercisesRotationsAndDeletionCases();
    testReservationPersistenceRebuildsTheIndex();
    return 0;
}
