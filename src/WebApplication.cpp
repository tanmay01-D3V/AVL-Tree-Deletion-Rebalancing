#include "WebApplication.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct SvgPoint {
    double x;
    double y;
};

struct TreeLayout {
    std::vector<SvgPoint> points;
    std::vector<std::pair<int, int>> edges;
    double width = 640;
    double height = 320;
};

std::string escapeHtml(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char character : value) {
        switch (character) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        case '\'':
            escaped += "&#39;";
            break;
        default:
            escaped += character;
            break;
        }
    }
    return escaped;
}

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int parseInteger(const std::string& rawValue, const char* fieldName) {
    const std::string value = trim(rawValue);
    if (value.empty()) {
        throw std::invalid_argument(std::string(fieldName) + " is required");
    }

    int number = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto result = std::from_chars(begin, end, number);
    if (result.ec != std::errc() || result.ptr != end) {
        throw std::invalid_argument(std::string(fieldName) + " must be an integer");
    }
    return number;
}

std::string requiredParam(const httplib::Request& request, const char* name) {
    if (!request.has_param(name)) {
        throw std::invalid_argument(std::string("Missing form field: ") + name);
    }
    return request.get_param_value(name);
}

Reservation readReservationForm(const httplib::Request& request) {
    ReservationStatus status;
    const std::string statusValue = requiredParam(request, "status");
    if (!parseReservationStatus(statusValue, status)) {
        throw std::invalid_argument("Choose Confirmed, Pending, or Cancelled");
    }

    return {
        parseInteger(requiredParam(request, "id"), "Reservation ID"),
        trim(requiredParam(request, "customer")),
        trim(requiredParam(request, "service")),
        trim(requiredParam(request, "date")),
        trim(requiredParam(request, "time")),
        parseInteger(requiredParam(request, "guests"), "Guests/resources"),
        status
    };
}

const char* rotationName(AVLRotationType type) {
    switch (type) {
    case AVLRotationType::Left:
        return "Left";
    case AVLRotationType::Right:
        return "Right";
    case AVLRotationType::LeftRight:
        return "Left-right";
    case AVLRotationType::RightLeft:
        return "Right-left";
    }
    return "Unknown";
}

std::string formatBalance(int value) {
    return value > 0 ? "+" + std::to_string(value) : std::to_string(value);
}

std::string statusClass(ReservationStatus status) {
    switch (status) {
    case ReservationStatus::Confirmed:
        return "confirmed";
    case ReservationStatus::Pending:
        return "pending";
    case ReservationStatus::Cancelled:
        return "cancelled";
    }
    return "pending";
}

TreeLayout makeTreeLayout(const std::vector<AVLTreeNodeSnapshot>& nodes,
                          int treeHeight) {
    TreeLayout layout;
    layout.points.resize(nodes.size());
    if (nodes.empty()) {
        return layout;
    }

    int inorderPosition = 0;
    int maximumDepth = 0;
    const auto assign = [&](const auto& self, int index, int depth) -> void {
        if (index < 0 || static_cast<std::size_t>(index) >= nodes.size()) {
            return;
        }
        const AVLTreeNodeSnapshot& node = nodes[static_cast<std::size_t>(index)];
        if (node.leftIndex != -1) {
            layout.edges.emplace_back(index, node.leftIndex);
            self(self, node.leftIndex, depth + 1);
        }
        layout.points[static_cast<std::size_t>(index)] = {
            58.0 + static_cast<double>(inorderPosition) * 92.0,
            52.0 + static_cast<double>(depth) * 104.0
        };
        ++inorderPosition;
        maximumDepth = std::max(maximumDepth, depth);
        if (node.rightIndex != -1) {
            layout.edges.emplace_back(index, node.rightIndex);
            self(self, node.rightIndex, depth + 1);
        }
    };
    assign(assign, 0, 0);

    layout.width = std::max(640.0, 116.0 + static_cast<double>(nodes.size()) * 92.0);
    layout.height = std::max(320.0, 130.0 + static_cast<double>(maximumDepth) * 104.0);
    if (treeHeight > 0 && maximumDepth + 1 != treeHeight) {
        throw std::runtime_error("Tree snapshot depth does not match stored AVL height");
    }
    return layout;
}

template <typename History>
std::string renderHistory(const History& history) {
    std::ostringstream html;
    html << "<ol class=\"history-list\">";
    std::size_t number = history.size();
    for (const auto& entry : history) {
        html << "<li class=\"history-item\"><span class=\"history-index\">"
             << (number < 10 ? "0" : "") << number--
             << "</span><div><span class=\"history-kind\">"
             << escapeHtml(entry.kind) << "</span><p>"
             << escapeHtml(entry.message) << "</p></div></li>";
    }
    html << "</ol>";
    return html.str();
}

std::string renderNotice(const std::string& notice, bool isError) {
    if (notice.empty()) {
        return {};
    }
    return "<p class=\"page-notice " + std::string(isError ? "notice-error" : "notice-success")
        + "\" role=\"status\">" + escapeHtml(notice) + "</p>";
}

std::string renderReservationFields(const Reservation* reservation) {
    const Reservation empty{0, {}, {}, {}, {}, 1, ReservationStatus::Pending};
    const Reservation& value = reservation == nullptr ? empty : *reservation;
    std::ostringstream html;
    html << "<label for=\"reservation-id\">Reservation ID</label>"
         << "<input id=\"reservation-id\" name=\"id\" type=\"number\" min=\"1\" "
         << "step=\"1\" required value=\""
         << (reservation == nullptr ? "" : std::to_string(value.id)) << "\">"
         << "<label for=\"customer\">Customer name</label>"
         << "<input id=\"customer\" name=\"customer\" type=\"text\" maxlength=\"100\" "
         << "required value=\"" << escapeHtml(value.customer) << "\">"
         << "<label for=\"service\">Service or resource</label>"
         << "<input id=\"service\" name=\"service\" type=\"text\" maxlength=\"100\" "
         << "required value=\"" << escapeHtml(value.service) << "\">"
         << "<label for=\"date\">Reservation date</label>"
         << "<input id=\"date\" name=\"date\" type=\"date\" required value=\""
         << escapeHtml(value.date) << "\">"
         << "<label for=\"time\">Reservation time</label>"
         << "<input id=\"time\" name=\"time\" type=\"time\" required value=\""
         << escapeHtml(value.time) << "\">"
         << "<label for=\"guests\">Guests / resources</label>"
         << "<input id=\"guests\" name=\"guests\" type=\"number\" min=\"1\" max=\"1000\" "
         << "step=\"1\" required value=\"" << value.guests << "\">"
         << "<label for=\"status\">Status</label><select id=\"status\" name=\"status\" required>";
    for (ReservationStatus status : {ReservationStatus::Confirmed,
                                     ReservationStatus::Pending,
                                     ReservationStatus::Cancelled}) {
        html << "<option value=\"" << reservationStatusName(status) << '"'
             << (value.status == status ? " selected" : "") << '>'
             << reservationStatusName(status) << "</option>";
    }
    html << "</select>";
    return html.str();
}

int childCount(const AVLTree& tree, int key) {
    for (const AVLTreeNodeSnapshot& node : tree.snapshot()) {
        if (node.key == key) {
            return (node.leftIndex != -1 ? 1 : 0)
                + (node.rightIndex != -1 ? 1 : 0);
        }
    }
    throw std::out_of_range("Reservation is not present in the AVL tree");
}

bool hasExactDemoKeys(const std::vector<Reservation>& reservations) {
    std::set<int> expected;
    for (const Reservation& reservation : reservationDemoDataset()) {
        expected.insert(reservation.id);
    }
    if (reservations.size() != expected.size()) {
        return false;
    }
    for (const Reservation& reservation : reservations) {
        if (expected.erase(reservation.id) != 1) {
            return false;
        }
    }
    return expected.empty();
}

} // namespace

std::string WebApplication::renderTreeSvg(
    const AVLTree& tree,
    const std::string& nodeLabel,
    std::optional<int> highlightedKey) const {
    const std::vector<AVLTreeNodeSnapshot> nodes = tree.snapshot();
    const TreeLayout layout = makeTreeLayout(nodes, tree.getHeight());
    std::ostringstream html;
    html << "<div class=\"tree-scroll\"><svg class=\"tree-svg\" viewBox=\"0 0 "
         << layout.width << ' ' << layout.height << "\" width=\"" << layout.width
         << "\" height=\"" << layout.height
         << "\" role=\"img\" aria-label=\"AVL tree with " << nodes.size()
         << ' ' << escapeHtml(nodeLabel) << " and height " << tree.getHeight() << ".\">"
         << "<title>AVL tree with " << nodes.size() << ' ' << escapeHtml(nodeLabel)
         << "; height " << tree.getHeight() << ".</title>";

    if (nodes.empty()) {
        html << "<text class=\"tree-empty-svg\" x=\"320\" y=\"160\" "
             << "text-anchor=\"middle\">Your tree is clear.</text>";
    } else {
        html << "<g class=\"tree-edges\" aria-hidden=\"true\">";
        for (const auto& edge : layout.edges) {
            const SvgPoint parent = layout.points[static_cast<std::size_t>(edge.first)];
            const SvgPoint child = layout.points[static_cast<std::size_t>(edge.second)];
            html << "<line x1=\"" << parent.x << "\" y1=\"" << parent.y
                 << "\" x2=\"" << child.x << "\" y2=\"" << child.y << "\"/>";
        }
        html << "</g><g class=\"tree-nodes\">";
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const AVLTreeNodeSnapshot& node = nodes[index];
            const SvgPoint point = layout.points[index];
            html << "<g class=\"tree-node"
                 << (highlightedKey == node.key ? " is-active" : "")
                 << "\" data-key=\"" << node.key << "\">"
                 << "<circle cx=\"" << point.x << "\" cy=\"" << point.y
                 << "\" r=\"25\"/><text class=\"node-key\" x=\"" << point.x
                 << "\" y=\"" << point.y - 3 << "\">" << node.key
                 << "</text><text class=\"node-balance\" x=\"" << point.x
                 << "\" y=\"" << point.y + 39 << "\">h=" << node.height
                 << " · bf " << formatBalance(node.balanceFactor)
                 << "</text></g>";
        }
        html << "</g>";
    }
    html << "</svg></div>";
    return html.str();
}

WebApplication::WebApplication(ReservationStore& reservations,
                               std::string assetDirectory)
    : reservations_(reservations),
      assetDirectory_(std::move(assetDirectory)) {
    loadVisualizerSample();
    const std::size_t count = reservations_.list().size();
    if (count > 0) {
        addHistory(reservationHistory_, "RESTORE",
                   "Restored " + std::to_string(count)
                       + " reservations and rebuilt the AVL index from SQLite.");
    }
}

void WebApplication::registerRoutes(httplib::Server& server) {
    server.Get("/styles.css", [this](const httplib::Request&, httplib::Response& response) {
        std::ifstream file(assetDirectory_ + "/styles.css", std::ios::binary);
        if (!file) {
            response.status = 404;
            response.set_content("Stylesheet not found", "text/plain; charset=UTF-8");
            return;
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        response.set_content(contents.str(), "text/css; charset=UTF-8");
    });

    server.Get("/favicon.svg", [this](const httplib::Request&, httplib::Response& response) {
        std::ifstream file(assetDirectory_ + "/favicon.svg", std::ios::binary);
        if (!file) {
            response.status = 404;
            response.set_content("Favicon not found", "text/plain; charset=UTF-8");
            return;
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        response.set_content(contents.str(), "image/svg+xml; charset=UTF-8");
    });

    server.Get("/", [this](const httplib::Request& request, httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleVisualizerGet(request, response);
    });
    server.Post("/visualizer/insert", [this](const httplib::Request& request,
                                              httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            const std::string rawKey = requiredParam(request, "key");
            const int key = parseInteger(rawKey, "Key");
            if (visualizerTree_.search(key)) {
                sendHtml(response,
                         renderVisualizer(std::to_string(key)
                                              + " is already present; duplicate keys are ignored.",
                                          true, rawKey, key),
                         409);
                return;
            }
            visualizerTree_.insert(key);
            addHistory(visualizerHistory_, "INSERT", "Inserted key #" + std::to_string(key) + ".");
            addRotationHistory(visualizerHistory_, visualizerTree_.takeRotationEvents());
            sendHtml(response,
                     renderVisualizer("Inserted " + std::to_string(key)
                                          + " using the C++ AVL tree.",
                                      false, {}, key));
        } catch (const std::invalid_argument& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 400);
        } catch (const std::exception& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 500);
        }
    });
    server.Post("/visualizer/delete", [this](const httplib::Request& request,
                                              httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            const std::string rawKey = requiredParam(request, "key");
            const int key = parseInteger(rawKey, "Key");
            if (!visualizerTree_.remove(key)) {
                sendHtml(response, renderVisualizer("Key #" + std::to_string(key)
                                                        + " was not found.",
                                                    true, rawKey),
                         404);
                return;
            }
            addHistory(visualizerHistory_, "DELETE", "Removed key #" + std::to_string(key) + ".");
            addRotationHistory(visualizerHistory_, visualizerTree_.takeRotationEvents());
            sendHtml(response,
                     renderVisualizer("Removed " + std::to_string(key)
                                          + " and rebalanced the affected path.",
                                      false));
        } catch (const std::invalid_argument& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 400);
        } catch (const std::exception& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 500);
        }
    });
    server.Post("/visualizer/reset", [this](const httplib::Request&, httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        loadVisualizerSample();
        sendHtml(response,
                 renderVisualizer("Restored the 15-key C++ sample tree."));
    });
    server.Post("/visualizer/example", [this](const httplib::Request& request,
                                               httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            const std::string pattern = requiredParam(request, "pattern");
            std::vector<int> sequence;
            if (pattern == "LL") {
                sequence = {30, 20, 10};
            } else if (pattern == "RR") {
                sequence = {10, 20, 30};
            } else if (pattern == "LR") {
                sequence = {30, 10, 20};
            } else if (pattern == "RL") {
                sequence = {10, 30, 20};
            } else {
                throw std::invalid_argument("Choose an LL, RR, LR, or RL rotation");
            }

            visualizerTree_.clear();
            visualizerHistory_.clear();
            for (int key : sequence) {
                visualizerTree_.insert(key);
                addHistory(visualizerHistory_, "INSERT",
                           "Inserted key #" + std::to_string(key) + ".");
                addRotationHistory(visualizerHistory_,
                                   visualizerTree_.takeRotationEvents());
            }
            sendHtml(response,
                     renderVisualizer(pattern
                                          + " example loaded; the displayed rotation came from C++."));
        } catch (const std::invalid_argument& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 400);
        } catch (const std::exception& error) {
            sendHtml(response, renderVisualizer(error.what(), true), 500);
        }
    });

    server.Get("/reservations", [this](const httplib::Request& request,
                                       httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::optional<ReservationStatus> filter;
        std::string notice;
        bool isError = false;
        if (request.has_param("status")) {
            ReservationStatus status;
            if (!parseReservationStatus(request.get_param_value("status"), status)) {
                notice = "Choose a valid reservation status filter.";
                isError = true;
                response.status = 400;
            } else {
                filter = status;
            }
        }
        sendHtml(response, renderReservations(filter, notice, isError), response.status);
    });
    server.Get("/reservations/search", [this](const httplib::Request& request,
                                               httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleReservationSearch(request, response);
    });
    server.Get("/reservations/view", [this](const httplib::Request& request,
                                             httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleReservationSearch(request, response);
    });
    server.Get("/reservations/edit", [this](const httplib::Request& request,
                                             httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            const int id = parseInteger(requiredParam(request, "id"), "Reservation ID");
            sendHtml(response, renderReservations(
                                   std::nullopt, "Editing reservation #" + std::to_string(id),
                                   false, reservations_.find(id)));
        } catch (const std::invalid_argument& error) {
            sendHtml(response, renderReservations(std::nullopt, error.what(), true), 400);
        } catch (const std::out_of_range& error) {
            sendHtml(response, renderReservations(std::nullopt, error.what(), true), 404);
        } catch (const std::exception& error) {
            sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
        }
    });
    server.Post("/reservations/create", [this](const httplib::Request& request,
                                                httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleReservationCreate(request, response);
    });
    server.Post("/reservations/update", [this](const httplib::Request& request,
                                                httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleReservationUpdate(request, response);
    });
    server.Post("/reservations/delete", [this](const httplib::Request& request,
                                                httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleReservationDelete(request, response);
    });
    server.Post("/reservations/demo/load", [this](const httplib::Request&,
                                                   httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleDemoLoad(response);
    });
    server.Post("/reservations/demo/run", [this](const httplib::Request&,
                                                  httplib::Response& response) {
        std::lock_guard<std::mutex> lock(mutex_);
        handleDemoRun(response);
    });
}

void WebApplication::loadVisualizerSample() {
    visualizerTree_.clear();
    visualizerHistory_.clear();
    for (int key : {44, 28, 63, 18, 35, 54, 76, 11, 22, 32, 39, 49, 59, 70, 83}) {
        visualizerTree_.insert(key);
        addHistory(visualizerHistory_, "INSERT", "Inserted key #" + std::to_string(key) + ".");
        addRotationHistory(visualizerHistory_, visualizerTree_.takeRotationEvents());
    }
    addHistory(visualizerHistory_, "READY", "15-key sample loaded by the C++ AVL tree.");
}

void WebApplication::addHistory(std::deque<HistoryEntry>& history,
                                std::string kind,
                                std::string message) {
    history.push_front({std::move(kind), std::move(message)});
    const std::size_t limit = &history == &visualizerHistory_ ? 12 : 40;
    if (history.size() > limit) {
        history.pop_back();
    }
}

void WebApplication::addRotationHistory(
    std::deque<HistoryEntry>& history,
    const std::vector<AVLRotationEvent>& events) {
    for (const AVLRotationEvent& event : events) {
        addHistory(history, "ROTATION",
                   std::string(rotationName(event.type)) + " rotation at #"
                       + std::to_string(event.pivotKey)
                       + (event.mutation == AVLMutationType::Insert
                              ? " while inserting #"
                              : " while deleting #")
                       + std::to_string(event.operationKey) + ".");
    }
}

std::string WebApplication::renderVisualizer(
    const std::string& notice,
    bool isError,
    const std::string& keyInput,
    std::optional<int> highlightedKey) const {
    const std::vector<AVLTreeNodeSnapshot> nodes = visualizerTree_.snapshot();
    const bool balanced = std::all_of(nodes.begin(), nodes.end(),
                                      [](const AVLTreeNodeSnapshot& node) {
                                          return std::abs(node.balanceFactor) <= 1;
                                      });
    std::ostringstream html;
    html << "<main id=\"top\" class=\"page-wrap\">"
         << "<section class=\"hero\"><div class=\"hero-copy\">"
         << "<span class=\"eyebrow hero-eyebrow\"><span class=\"eyebrow-line\"></span>"
         << "An interactive field guide to AVL trees</span>"
         << "<h1>Keep the tree.<br><em>Lose the imbalance.</em></h1>"
         << "<p class=\"hero-description\">Add a key. Watch the branches shift. "
         << "Every operation runs in the existing C++ AVL implementation.</p>"
         << "<a class=\"hero-cta\" href=\"#visualizer\">Explore the live tree <span>↓</span></a>"
         << "</div><aside class=\"hero-note\" aria-label=\"AVL balance rule\">"
         << "<div class=\"note-topline\"><span class=\"note-index\">FIELD NOTE / 01</span>"
         << "<span class=\"note-spark\" aria-hidden=\"true\">✳</span></div>"
         << "<p class=\"note-label\">The balance rule</p><div class=\"balance-equation\">"
         << "<span>−1</span><span class=\"equation-track\"><i></i><b></b><i></i></span>"
         << "<span>+1</span></div><p class=\"note-copy\">Every node keeps its left and right "
         << "subtree heights within one. When it doesn’t, the C++ tree rotates.</p></aside></section>"
         << "<section class=\"lab-section\" id=\"visualizer\"><div class=\"section-heading\">"
         << "<div><span class=\"eyebrow\">01 — Live workspace</span>"
         << "<h2>Shape the structure.</h2></div><p>Server-rendered HTML.<br>"
         << "The C++ tree is the source of every result.</p></div>"
         << renderNotice(notice, isError)
         << "<div class=\"workspace\"><aside class=\"control-panel\" aria-labelledby=\"operations-title\">"
         << "<div class=\"panel-heading\"><div><span class=\"eyebrow\">01 / Make a move</span>"
         << "<h2 id=\"operations-title\">Tree controls</h2></div>"
         << "<form method=\"post\" action=\"/visualizer/reset\">"
         << "<button class=\"icon-button\" type=\"submit\" aria-label=\"Reset sample tree\">↻</button>"
         << "</form></div>"
         << "<form class=\"operation-form\" method=\"post\" action=\"/visualizer/insert\">"
         << "<label class=\"field-label\" for=\"insert-key\">Integer key</label>"
         << "<div class=\"input-wrap\"><span aria-hidden=\"true\">#</span>"
         << "<input id=\"insert-key\" name=\"key\" type=\"number\" step=\"1\" required value=\""
         << escapeHtml(keyInput) << "\" placeholder=\"e.g. 42\"></div>"
         << "<button class=\"action-button action-primary\" type=\"submit\">Insert key</button>"
         << "</form><form class=\"operation-form compact-form\" method=\"post\" "
         << "action=\"/visualizer/delete\"><label class=\"field-label\" for=\"delete-key\">"
         << "Delete a key</label><div class=\"input-wrap\"><span>#</span><input id=\"delete-key\" "
         << "name=\"key\" type=\"number\" step=\"1\" required placeholder=\"Key\"></div>"
         << "<button class=\"action-button action-secondary\" type=\"submit\">Delete</button></form>"
         << "<form class=\"operation-form compact-form\" method=\"get\" action=\"/\">"
         << "<label class=\"field-label\" for=\"search-key\">Search with C++</label>"
         << "<div class=\"input-wrap\"><span>#</span><input id=\"search-key\" name=\"search\" "
         << "type=\"number\" step=\"1\" required placeholder=\"Key\"></div>"
         << "<button class=\"action-button action-secondary\" type=\"submit\">Find</button></form>"
         << "<div class=\"panel-divider\"></div><div class=\"example-block\">"
         << "<span class=\"eyebrow\">Rotation examples</span>"
         << "<p>Choose a pattern; C++ inserts its sequence and reports only rotations performed.</p>"
         << "<div class=\"rotation-buttons\">";
    for (const char* pattern : {"LL", "RR", "LR", "RL"}) {
        html << "<form method=\"post\" action=\"/visualizer/example\">"
             << "<button class=\"rotation-button\" type=\"submit\" name=\"pattern\" value=\""
             << pattern << "\">" << pattern << "</button></form>";
    }
    html << "</div></div></aside>"
         << "<section class=\"tree-card\" aria-labelledby=\"tree-title\">"
         << "<div class=\"tree-card-head\"><div><span class=\"eyebrow\">02 / Structure view</span>"
         << "<h2 id=\"tree-title\">Live tree</h2></div><span class=\"balance-pill "
         << (balanced ? "is-balanced" : "is-unbalanced") << "\"><span></span>"
         << (balanced ? "Balanced" : "Needs rebalance")
         << "</span></div><div class=\"tree-stage\"><div class=\"stage-grid\"></div>"
         << "<span class=\"stage-label stage-label-left\">ROOT → LEAVES</span>"
         << "<span class=\"stage-label stage-label-right\">HEIGHT "
         << visualizerTree_.getHeight() << "</span>"
         << renderTreeSvg(visualizerTree_, "keys", highlightedKey)
         << "<div class=\"stage-footer\"><span><i class=\"legend-dot\"></i>Node</span>"
         << "<span>height / balance factor</span><span class=\"stage-footer-note\">"
         << "bf = height(left) − height(right)</span></div></div>"
         << "<div class=\"inorder-strip\"><span class=\"eyebrow\">Inorder / sorted keys</span><p>";
    const std::vector<int> inorder = visualizerTree_.inorderTraversal();
    if (inorder.empty()) {
        html << "—";
    } else {
        for (std::size_t index = 0; index < inorder.size(); ++index) {
            if (index > 0) {
                html << " · ";
            }
            html << inorder[index];
        }
    }
    html << "</p></div></section><aside class=\"inspector\" aria-label=\"Tree metrics and history\">"
         << "<section class=\"metrics-grid\"><div class=\"metric-card\"><span class=\"eyebrow\">Nodes</span>"
         << "<strong>" << nodes.size() << "</strong><span class=\"metric-detail\">unique keys</span></div>"
         << "<div class=\"metric-card\"><span class=\"eyebrow\">Height</span><strong>"
         << visualizerTree_.getHeight() << "</strong><span class=\"metric-detail\">levels deep</span></div>"
         << "<div class=\"metric-card\"><span class=\"eyebrow\">Root bf</span><strong>"
         << formatBalance(visualizerTree_.getBalanceFactor())
         << "</strong><span class=\"metric-detail\">left − right</span></div></section>"
         << "<section class=\"history-card\"><div class=\"panel-heading history-heading\"><div>"
         << "<span class=\"eyebrow\">03 / Recent changes</span><h2>Operation log</h2></div>"
         << "<span class=\"history-count\">" << visualizerHistory_.size() << "</span></div>"
         << renderHistory(visualizerHistory_) << "</section>"
         << "<p class=\"inspector-footnote\">↳ All tree operations and rotation reports come from C++.</p>"
         << "</aside></div></section>"
         << "<section class=\"principles-section\" id=\"principles\"><div class=\"principles-intro\">"
         << "<span class=\"eyebrow\">02 — The short version</span>"
         << "<h2>Order, height, <em>harmony.</em></h2>"
         << "<p>Three simple ideas. One tree that stays quick to explore.</p></div>"
         << "<div class=\"principle-grid\"><article class=\"principle-card\"><span class=\"principle-number\">01</span>"
         << "<h3>Keep the order</h3><p>Smaller keys live left; larger keys live right.</p></article>"
         << "<article class=\"principle-card\"><span class=\"principle-number\">02</span>"
         << "<h3>Measure the height</h3><p>Every node stores the height of its tallest child path.</p></article>"
         << "<article class=\"principle-card principle-card-accent\"><span class=\"principle-number\">03</span>"
         << "<h3>Rotate when needed</h3><p>Rebalancing preserves sorted order while restoring AVL balance.</p>"
         << "</article></div></section></main>";
    return renderPage("Interactive AVL Visualizer", "visualizer", html.str());
}

std::string WebApplication::renderReservations(
    std::optional<ReservationStatus> filter,
    const std::string& notice,
    bool isError,
    std::optional<Reservation> editReservation,
    std::optional<Reservation> detailReservation) const {
    const std::vector<Reservation> allReservations = reservations_.list();
    const std::vector<Reservation> visibleReservations =
        filter.has_value() ? reservations_.list(filter) : allReservations;
    if (!reservations_.verifySynchronization()) {
        throw std::runtime_error("Reservation database and AVL index are out of sync");
    }

    const auto countStatus = [&allReservations](ReservationStatus status) {
        return std::count_if(allReservations.begin(), allReservations.end(),
                             [status](const Reservation& reservation) {
                                 return reservation.status == status;
                             });
    };
    const std::vector<AVLTreeNodeSnapshot> nodes = reservations_.index().snapshot();
    const bool balanced = std::all_of(nodes.begin(), nodes.end(),
                                      [](const AVLTreeNodeSnapshot& node) {
                                          return std::abs(node.balanceFactor) <= 1;
                                      });
    const std::vector<Reservation> demo = reservationDemoDataset();
    const bool exactDemo = hasExactDemoKeys(allReservations);
    const bool demoIdsAvailable = std::all_of(
        demo.begin(), demo.end(), [&allReservations](const Reservation& sample) {
            return std::none_of(allReservations.begin(), allReservations.end(),
                                [&sample](const Reservation& reservation) {
                                    return reservation.id == sample.id;
                                });
        });

    std::ostringstream html;
    html << "<main id=\"top\" class=\"page-wrap reservation-page\">"
         << "<section class=\"reservation-hero\"><div><span class=\"eyebrow\">"
         << "AVL STUDIO / RESERVATION INDEX</span><h1>Make room for <em>what matters.</em></h1>"
         << "<p>A practical reservation desk, indexed and balanced by the same C++ AVL tree.</p>"
         << "</div><a class=\"hero-cta\" href=\"#reservation-form\">Create a reservation <span>↓</span></a>"
         << "</section>" << renderNotice(notice, isError)
         << "<section class=\"reservation-metrics\" aria-label=\"Reservation statistics\">";
    const auto metric = [&html](const char* label, std::size_t value, const char* detail) {
        html << "<div class=\"metric-card\"><span class=\"eyebrow\">" << label
             << "</span><strong>" << value << "</strong><span class=\"metric-detail\">"
             << detail << "</span></div>";
    };
    metric("Total", allReservations.size(), "reservations");
    metric("Confirmed", static_cast<std::size_t>(countStatus(ReservationStatus::Confirmed)),
           "booked");
    metric("Pending", static_cast<std::size_t>(countStatus(ReservationStatus::Pending)),
           "awaiting");
    metric("Cancelled", static_cast<std::size_t>(countStatus(ReservationStatus::Cancelled)),
           "released");
    metric("AVL height", static_cast<std::size_t>(reservations_.index().getHeight()),
           "levels");
    html << "<div class=\"metric-card\"><span class=\"eyebrow\">Index health</span><strong class=\"health-value\">"
         << (balanced ? "OK" : "CHECK") << "</strong><span class=\"metric-detail\">"
         << (balanced ? "balanced" : "needs attention") << "</span></div>"
         << "</section><section class=\"reservation-workspace\">"
         << "<div class=\"reservation-main\"><section class=\"tree-card reservation-tree-card\">"
         << "<div class=\"tree-card-head\"><div><span class=\"eyebrow\">01 / Live index</span>"
         << "<h2>Reservation ID tree</h2></div><span class=\"balance-pill "
         << (balanced ? "is-balanced" : "is-unbalanced") << "\"><span></span>"
         << (balanced ? "Balanced" : "Needs rebalance") << "</span></div>"
         << "<div class=\"tree-stage\"><div class=\"stage-grid\"></div>"
         << "<span class=\"stage-label stage-label-left\">RESERVATION ID → LEAVES</span>"
         << "<span class=\"stage-label stage-label-right\">HEIGHT "
         << reservations_.index().getHeight() << " · ROOT BF "
         << formatBalance(reservations_.index().getBalanceFactor()) << "</span>"
         << renderTreeSvg(reservations_.index(), "reservation IDs")
         << "<div class=\"stage-footer\"><span>h = stored node height</span>"
         << "<span>bf = height(left) − height(right)</span>"
         << "<span class=\"stage-footer-note\">Generated from the live C++ index</span></div></div>"
         << "</section>";
    if (detailReservation.has_value()) {
        const Reservation& reservation = *detailReservation;
        html << "<section class=\"detail-card\"><span class=\"eyebrow\">"
             << "RESERVATION / #" << reservation.id << "</span><h2>"
             << escapeHtml(reservation.customer) << "</h2><p>"
             << escapeHtml(reservation.service) << " · " << escapeHtml(reservation.date)
             << " at " << escapeHtml(reservation.time) << " · " << reservation.guests
             << " guests/resources · <span class=\"status-pill "
             << statusClass(reservation.status) << "\">"
             << reservationStatusName(reservation.status) << "</span></p></section>";
    }
    html << "<section class=\"table-card\"><div class=\"table-heading\"><div>"
         << "<span class=\"eyebrow\">02 / Reservation ledger</span><h2>Reservations</h2></div>"
         << "<form class=\"search-form\" method=\"get\" action=\"/reservations/search\">"
         << "<label class=\"visually-hidden\" for=\"reservation-search\">Search reservation ID</label>"
         << "<input id=\"reservation-search\" name=\"id\" type=\"number\" min=\"1\" step=\"1\" "
         << "placeholder=\"Find ID\" required><button type=\"submit\">Find</button></form></div>"
         << "<nav class=\"filter-bar\" aria-label=\"Filter reservations\">"
         << "<a href=\"/reservations\""
         << (!filter.has_value() ? " aria-current=\"page\"" : "") << ">All</a>";
    for (ReservationStatus status : {ReservationStatus::Confirmed,
                                     ReservationStatus::Pending,
                                     ReservationStatus::Cancelled}) {
        html << "<a href=\"/reservations?status=" << reservationStatusName(status) << '"'
             << (filter == status ? " aria-current=\"page\"" : "") << '>'
             << reservationStatusName(status) << "</a>";
    }
    html << "</nav><div class=\"table-scroll\"><table><thead><tr>"
         << "<th>ID</th><th>Customer</th><th>Service</th><th>Date</th><th>Time</th>"
         << "<th>Guests</th><th>Status</th><th>Actions</th></tr></thead><tbody>";
    if (visibleReservations.empty()) {
        html << "<tr><td class=\"empty-table\" colspan=\"8\">No reservations match this view.</td></tr>";
    } else {
        for (const Reservation& reservation : visibleReservations) {
            html << "<tr><td class=\"mono-cell\"><a href=\"/reservations/view?id="
                 << reservation.id << "\">#" << reservation.id << "</a></td><td>"
                 << escapeHtml(reservation.customer) << "</td><td>"
                 << escapeHtml(reservation.service) << "</td><td>"
                 << escapeHtml(reservation.date) << "</td><td>"
                 << escapeHtml(reservation.time) << "</td><td>" << reservation.guests
                 << "</td><td><span class=\"status-pill " << statusClass(reservation.status)
                 << "\">" << reservationStatusName(reservation.status)
                 << "</span></td><td class=\"table-actions\"><a href=\"/reservations/edit?id="
                 << reservation.id << "\">Edit</a><form method=\"post\" action=\"/reservations/delete\">"
                 << "<input type=\"hidden\" name=\"id\" value=\"" << reservation.id
                 << "\"><button class=\"link-button delete-link\" type=\"submit\">Delete</button>"
                 << "</form></td></tr>";
        }
    }
    html << "</tbody></table></div></section></div><aside class=\"reservation-rail\">"
         << "<section class=\"control-panel reservation-form-card\" id=\"reservation-form\">"
         << "<span class=\"eyebrow\">" << (editReservation.has_value()
                                                ? "03 / Edit reservation"
                                                : "03 / New reservation")
         << "</span><h2>" << (editReservation.has_value()
                                 ? "Update the details."
                                 : "Reserve a place.") << "</h2>"
         << "<form class=\"reservation-form\" method=\"post\" action=\""
         << (editReservation.has_value() ? "/reservations/update"
                                         : "/reservations/create") << "\">";
    if (editReservation.has_value()) {
        html << "<input type=\"hidden\" name=\"old_id\" value=\""
             << editReservation->id << "\">";
    }
    html << renderReservationFields(editReservation.has_value() ? &*editReservation : nullptr)
         << "<button class=\"action-button action-primary\" type=\"submit\">"
         << (editReservation.has_value() ? "Save changes" : "Create reservation")
         << "</button>";
    if (editReservation.has_value()) {
        html << "<a class=\"cancel-link\" href=\"/reservations\">Cancel edit</a>";
    }
    html << "</form></section><section class=\"history-card reservation-history\">"
         << "<div class=\"panel-heading history-heading\"><div><span class=\"eyebrow\">"
         << "04 / Server-side record</span><h2>Operation history</h2></div>"
         << "<span class=\"history-count\">" << reservationHistory_.size()
         << "</span></div>" << renderHistory(reservationHistory_) << "</section>"
         << "<section class=\"demo-card\"><span class=\"eyebrow\">"
         << "05 / Verified demonstration</span><h2>Watch the index work.</h2>"
         << "<p>The sample inserts trigger all four rotation patterns. Its next steps search "
         << "#1011, delete leaf #1004 (two ancestor rotations), one-child #1006, then "
         << "two-child #1008.</p>";
    if (exactDemo) {
        html << "<form method=\"post\" action=\"/reservations/demo/run\">"
             << "<button class=\"action-button action-secondary\" type=\"submit\">"
             << "Run verified demo sequence</button></form>";
    } else if (demoIdsAvailable && allReservations.empty()) {
        html << "<form method=\"post\" action=\"/reservations/demo/load\">"
             << "<button class=\"action-button action-secondary\" type=\"submit\">"
             << "Load sample reservations</button></form>";
    } else {
        html << "<p class=\"demo-note\">Load the sample on an empty reservation index to "
             << "keep its structural cases reproducible. Existing reservations are never replaced.</p>";
    }
    html << "</section></aside></section></main>";
    return renderPage("Reservation Management", "reservations", html.str());
}

std::string WebApplication::renderPage(const std::string& title,
                                       const std::string& activePage,
                                       const std::string& content) const {
    std::ostringstream html;
    html << "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
         << "<meta name=\"theme-color\" content=\"#f4f1e8\">"
         << "<meta name=\"description\" content=\"AVL Studio: a C++ AVL visualizer and reservation manager.\">"
         << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">"
         << "<link rel=\"stylesheet\" href=\"/styles.css\"><title>"
         << escapeHtml(title) << " · AVL Studio</title></head><body>"
         << "<a class=\"skip-link\" href=\"#top\">Skip to the main content</a>"
         << "<div class=\"site-shell\"><header class=\"topbar page-wrap\">"
         << "<a class=\"brand\" href=\"/\" aria-label=\"AVL Studio home\">"
         << "<span class=\"brand-mark\" aria-hidden=\"true\"><svg viewBox=\"0 0 34 34\">"
         << "<path d=\"M17 7v7m0 0-8 7m8-7 8 7M9 21v6m16-6v6\"/>"
         << "<circle cx=\"17\" cy=\"6\" r=\"3\"/><circle cx=\"8\" cy=\"21\" r=\"3\"/>"
         << "<circle cx=\"26\" cy=\"21\" r=\"3\"/></svg></span>"
         << "<span class=\"brand-copy\"><strong>AVL <em>Studio</em></strong>"
         << "<small>STRUCTURE / IN MOTION</small></span></a>"
         << "<nav class=\"topnav\" aria-label=\"Main navigation\"><a href=\"/\""
         << (activePage == "visualizer" ? " aria-current=\"page\"" : "")
         << ">AVL visualizer</a><a href=\"/reservations\""
         << (activePage == "reservations" ? " aria-current=\"page\"" : "")
         << ">Reservations</a></nav><a class=\"project-link\" "
         << "href=\"https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing\">"
         << "<span class=\"status-light\" aria-hidden=\"true\"></span>C++ project ↗</a>"
         << "</header>" << content << "<footer class=\"footer page-wrap\">"
         << "<a class=\"footer-brand\" href=\"/\">AVL <em>Studio</em></a>"
         << "<p>A C++ AVL tree, brought to the web with server-rendered HTML and SVG.</p>"
         << "<a href=\"https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing\">"
         << "View source ↗</a></footer></div></body></html>";
    return html.str();
}

void WebApplication::sendHtml(httplib::Response& response,
                              const std::string& html,
                              int status) const {
    response.status = status;
    response.set_header("X-Content-Type-Options", "nosniff");
    response.set_header("Referrer-Policy", "same-origin");
    response.set_header("Content-Security-Policy",
                        "default-src 'self'; script-src 'none'; style-src 'self'; "
                        "img-src 'self' data:; form-action 'self'; base-uri 'self'; "
                        "frame-ancestors 'none'");
    response.set_content(html, "text/html; charset=UTF-8");
}

void WebApplication::handleVisualizerGet(const httplib::Request& request,
                                          httplib::Response& response) {
    std::string notice;
    bool isError = false;
    std::string keyInput;
    std::optional<int> highlightedKey;
    if (request.has_param("search")) {
        keyInput = request.get_param_value("search");
        try {
            const int key = parseInteger(keyInput, "Key");
            if (visualizerTree_.search(key)) {
                notice = "Key #" + std::to_string(key) + " was found in the C++ AVL tree.";
                highlightedKey = key;
                addHistory(visualizerHistory_, "SEARCH", "Found key #" + std::to_string(key) + ".");
            } else {
                notice = "Key #" + std::to_string(key) + " was not found.";
                isError = true;
                addHistory(visualizerHistory_, "SEARCH",
                           "Key #" + std::to_string(key) + " was not found.");
            }
        } catch (const std::invalid_argument& error) {
            notice = error.what();
            isError = true;
            response.status = 400;
        }
    }
    sendHtml(response, renderVisualizer(notice, isError, keyInput, highlightedKey),
             response.status);
}

void WebApplication::handleReservationSearch(const httplib::Request& request,
                                              httplib::Response& response) {
    try {
        const int id = parseInteger(requiredParam(request, "id"), "Reservation ID");
        const Reservation reservation = reservations_.find(id);
        addHistory(reservationHistory_, "SEARCH",
                   "Reservation #" + std::to_string(id) + " found in the AVL index.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Reservation #" + std::to_string(id) + " found.",
                                    false, std::nullopt, reservation));
    } catch (const std::invalid_argument& error) {
        addHistory(reservationHistory_, "VALIDATION", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 400);
    } catch (const std::out_of_range& error) {
        const std::string id = request.has_param("id") ? request.get_param_value("id") : "?";
        addHistory(reservationHistory_, "SEARCH",
                   "Reservation #" + id + " was not found in the AVL index.");
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 404);
    } catch (const std::exception& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}

void WebApplication::handleReservationCreate(const httplib::Request& request,
                                              httplib::Response& response) {
    try {
        const Reservation reservation = readReservationForm(request);
        reservations_.create(reservation);
        addHistory(reservationHistory_, "INSERT",
                   "Reservation #" + std::to_string(reservation.id) + " inserted.");
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());
        addHistory(reservationHistory_, "VALIDATION",
                   "BST ordering, stored heights, and AVL balance passed.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Reservation #" + std::to_string(reservation.id)
                                        + " created and indexed by C++."));
    } catch (const std::invalid_argument& error) {
        addHistory(reservationHistory_, "VALIDATION", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 400);
    } catch (const std::exception& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}

void WebApplication::handleReservationUpdate(const httplib::Request& request,
                                              httplib::Response& response) {
    try {
        const int oldId = parseInteger(requiredParam(request, "old_id"), "Original ID");
        const Reservation reservation = readReservationForm(request);
        reservations_.update(oldId, reservation);
        addHistory(reservationHistory_, "UPDATE",
                   "Reservation #" + std::to_string(oldId) + " updated"
                       + (oldId == reservation.id
                              ? "."
                              : " and reindexed as #" + std::to_string(reservation.id) + "."));
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());
        addHistory(reservationHistory_, "VALIDATION",
                   "Reservation records and AVL keys remain synchronized.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Reservation #" + std::to_string(reservation.id)
                                        + " updated."));
    } catch (const std::invalid_argument& error) {
        addHistory(reservationHistory_, "VALIDATION", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 400);
    } catch (const std::out_of_range& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 404);
    } catch (const std::exception& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}

void WebApplication::handleReservationDelete(const httplib::Request& request,
                                              httplib::Response& response) {
    try {
        const int id = parseInteger(requiredParam(request, "id"), "Reservation ID");
        reservations_.remove(id);
        addHistory(reservationHistory_, "DELETE",
                   "Reservation #" + std::to_string(id) + " removed from SQLite and AVL.");
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());
        addHistory(reservationHistory_, "VALIDATION",
                   "BST ordering, stored heights, and AVL balance passed.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Reservation #" + std::to_string(id) + " deleted."));
    } catch (const std::invalid_argument& error) {
        addHistory(reservationHistory_, "VALIDATION", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 400);
    } catch (const std::out_of_range& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 404);
    } catch (const std::exception& error) {
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}

void WebApplication::handleDemoLoad(httplib::Response& response) {
    const std::vector<Reservation> samples = reservationDemoDataset();
    if (!reservations_.list().empty()) {
        sendHtml(response,
                 renderReservations(
                     std::nullopt,
                     "Load the verified demo only into an empty index; existing reservations were left untouched.",
                     true),
                 409);
        return;
    }

    try {
        reservations_.createMany(samples);
        const std::vector<AVLRotationEvent> rotations =
            reservations_.takeRotationEvents();
        addHistory(reservationHistory_, "DEMO",
                   "Loaded 12 verified sample reservations in C++ insertion order.");
        for (const Reservation& reservation : samples) {
            addHistory(reservationHistory_, "INSERT",
                       "Reservation #" + std::to_string(reservation.id) + " inserted.");
            for (const AVLRotationEvent& event : rotations) {
                if (event.operationKey == reservation.id) {
                    addRotationHistory(reservationHistory_, {event});
                }
            }
        }
        addHistory(reservationHistory_, "VALIDATION",
                   "Sample IDs, BST ordering, stored heights, and AVL balance passed.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Loaded the verified 12-reservation sample. Run the sequence to search and delete the documented cases."));
    } catch (const std::exception& error) {
        addHistory(reservationHistory_, "VALIDATION", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}

void WebApplication::handleDemoRun(httplib::Response& response) {
    const std::vector<Reservation> current = reservations_.list();
    if (!hasExactDemoKeys(current)) {
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "The verified sequence requires the unchanged 12-key sample dataset. No records were modified.",
                                    true),
                 409);
        return;
    }

    try {
        const Reservation found = reservations_.find(1011);
        addHistory(reservationHistory_, "SEARCH",
                   "Reservation #" + std::to_string(found.id) + " found in the AVL index.");

        if (childCount(reservations_.index(), 1004) != 0) {
            throw std::runtime_error("Sample precondition failed: #1004 is not a leaf");
        }
        reservations_.remove(1004);
        addHistory(reservationHistory_, "DELETE", "Reservation #1004 leaf removed.");
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());

        if (childCount(reservations_.index(), 1006) != 1) {
            throw std::runtime_error("Sample precondition failed: #1006 is not a one-child node");
        }
        reservations_.remove(1006);
        addHistory(reservationHistory_, "DELETE", "Reservation #1006 one-child node removed.");
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());

        if (childCount(reservations_.index(), 1008) != 2) {
            throw std::runtime_error("Sample precondition failed: #1008 is not a two-child node");
        }
        reservations_.remove(1008);
        addHistory(reservationHistory_, "DELETE", "Reservation #1008 two-child node removed.");
        addRotationHistory(reservationHistory_, reservations_.takeRotationEvents());
        addHistory(reservationHistory_, "VALIDATION",
                   "The multi-ancestor deletion, one-child deletion, and two-child deletion passed.");
        sendHtml(response,
                 renderReservations(std::nullopt,
                                    "Demo complete: search found #1011; leaf #1004 rebalanced two ancestors; #1006 and #1008 exercised one-child and two-child deletion."));
    } catch (const std::exception& error) {
        addHistory(reservationHistory_, "DEMO ERROR", error.what());
        sendHtml(response, renderReservations(std::nullopt, error.what(), true), 500);
    }
}
