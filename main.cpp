#include "AVLTree.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class ReadResult {
    Value,
    Invalid,
    EndOfInput
};

ReadResult readInteger(const std::string& prompt, int& value) {
    std::cout << prompt;

    std::string line;
    if (!std::getline(std::cin, line)) {
        return ReadResult::EndOfInput;
    }

    std::istringstream input(line);
    char extra;
    if (!(input >> value) || (input >> extra)) {
        return ReadResult::Invalid;
    }

    return ReadResult::Value;
}

void printMenu() {
    std::cout << "\n=== AVL Tree Menu ===\n"
              << "1. Insert a value\n"
              << "2. Delete a value\n"
              << "3. Search for a value\n"
              << "4. Display inorder traversal\n"
              << "5. Display preorder traversal\n"
              << "6. Display postorder traversal\n"
              << "7. Display tree height\n"
              << "8. Display root balance factor\n"
              << "9. Display tree contents (inorder)\n"
              << "10. Exit\n";
}

void printTraversal(const char* label, const std::vector<int>& values) {
    if (values.empty()) {
        std::cout << "The tree is empty.\n";
        return;
    }

    std::cout << label;
    for (int value : values) {
        std::cout << value << ' ';
    }
    std::cout << '\n';
}

bool readValue(int& value) {
    const ReadResult result = readInteger("Enter an integer value: ", value);
    if (result == ReadResult::Invalid) {
        std::cout << "Invalid input. Please enter a whole number.\n";
        return false;
    }
    return result == ReadResult::Value;
}

} // namespace

int main() {
    AVLTree tree;

    while (true) {
        printMenu();

        int choice = 0;
        const ReadResult result = readInteger("Choose an option (1-10): ", choice);
        if (result == ReadResult::EndOfInput) {
            std::cout << "\nInput closed. Exiting.\n";
            break;
        }
        if (result == ReadResult::Invalid || choice < 1 || choice > 10) {
            std::cout << "Invalid choice. Enter a number from 1 to 10.\n";
            continue;
        }

        if (choice == 10) {
            std::cout << "Goodbye.\n";
            break;
        }

        if (choice == 1) {
            int value = 0;
            if (!readValue(value)) {
                if (!std::cin) {
                    std::cout << "\nInput closed. Exiting.\n";
                    break;
                }
                continue;
            }
            if (tree.search(value)) {
                std::cout << value << " is already in the tree; duplicate ignored.\n";
            } else {
                tree.insert(value);
                std::cout << "Inserted " << value << ".\n";
            }
        } else if (choice == 2) {
            int value = 0;
            if (!readValue(value)) {
                if (!std::cin) {
                    std::cout << "\nInput closed. Exiting.\n";
                    break;
                }
                continue;
            }
            if (tree.remove(value)) {
                std::cout << "Deleted " << value << ".\n";
            } else {
                std::cout << value << " was not found; nothing was deleted.\n";
            }
        } else if (choice == 3) {
            int value = 0;
            if (!readValue(value)) {
                if (!std::cin) {
                    std::cout << "\nInput closed. Exiting.\n";
                    break;
                }
                continue;
            }
            if (tree.search(value)) {
                std::cout << value << " was found in the tree.\n";
            } else {
                std::cout << value << " was not found in the tree.\n";
            }
        } else if (choice == 4) {
            printTraversal("Inorder traversal: ", tree.inorderTraversal());
        } else if (choice == 5) {
            printTraversal("Preorder traversal: ", tree.preorderTraversal());
        } else if (choice == 6) {
            printTraversal("Postorder traversal: ", tree.postorderTraversal());
        } else if (choice == 7) {
            const int height = tree.getHeight();
            if (height == 0) {
                std::cout << "The tree is empty. Height: 0.\n";
            } else {
                std::cout << "Tree height: " << height << ".\n";
            }
        } else if (choice == 8) {
            if (tree.getHeight() == 0) {
                std::cout << "The tree is empty; root balance factor: 0.\n";
            } else {
                std::cout << "Root balance factor: " << tree.getBalanceFactor()
                          << " (height(left) - height(right)).\n";
            }
        } else if (choice == 9) {
            printTraversal("Tree contents (inorder): ", tree.inorderTraversal());
        }
    }

    return 0;
}
