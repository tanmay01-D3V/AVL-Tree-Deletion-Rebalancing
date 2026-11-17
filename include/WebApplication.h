#ifndef WEB_APPLICATION_H
#define WEB_APPLICATION_H

#include "AVLTree.h"
#include "ReservationStore.h"

#include <httplib.h>

#include <deque>
#include <mutex>
#include <optional>
#include <string>

class WebApplication {
public:
    WebApplication(ReservationStore& reservations, std::string assetDirectory);
    void registerRoutes(httplib::Server& server);

private:
    struct HistoryEntry {
        std::string kind;
        std::string message;
    };

    ReservationStore& reservations_;
    std::string assetDirectory_;
    AVLTree visualizerTree_;
    std::deque<HistoryEntry> visualizerHistory_;
    std::deque<HistoryEntry> reservationHistory_;
    std::mutex mutex_;

    void loadVisualizerSample();
    void addHistory(std::deque<HistoryEntry>& history,
                    std::string kind,
                    std::string message);
    void addRotationHistory(std::deque<HistoryEntry>& history,
                            const std::vector<AVLRotationEvent>& events);

    std::string renderVisualizer(const std::string& notice = {},
                                 bool isError = false,
                                 const std::string& keyInput = {},
                                 std::optional<int> highlightedKey = std::nullopt) const;
    std::string renderReservations(
        std::optional<ReservationStatus> filter = std::nullopt,
        const std::string& notice = {},
        bool isError = false,
        std::optional<Reservation> editReservation = std::nullopt,
        std::optional<Reservation> detailReservation = std::nullopt) const;
    std::string renderTreeSvg(const AVLTree& tree,
                              const std::string& nodeLabel,
                              std::optional<int> highlightedKey = std::nullopt) const;
    std::string renderPage(const std::string& title,
                           const std::string& activePage,
                           const std::string& content) const;

    void sendHtml(httplib::Response& response,
                  const std::string& html,
                  int status = 200) const;
    void handleVisualizerGet(const httplib::Request& request,
                             httplib::Response& response);
    void handleReservationSearch(const httplib::Request& request,
                                 httplib::Response& response);
    void handleReservationCreate(const httplib::Request& request,
                                 httplib::Response& response);
    void handleReservationUpdate(const httplib::Request& request,
                                 httplib::Response& response);
    void handleReservationDelete(const httplib::Request& request,
                                 httplib::Response& response);
    void handleDemoLoad(httplib::Response& response);
    void handleDemoRun(httplib::Response& response);
};

#endif
