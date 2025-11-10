#include <iostream>
#include <string>
#include "CppInterOp/CppInterOp.h"

// Minimal smoke test to verify CppInterOp functionality
int main() {
    try {
        // Initialize a minimal interpreter instance
        cppinterop::Interpreter I;
        
        // Try to evaluate a simple expression
        const char* code = "int answer = 42;";
        if (!I.declare(code)) {
            std::cerr << "Failed to declare variable\n";
            return 1;
        }

        // Verify we can retrieve the value
        int result = 0;
        if (!I.evaluate("answer", result)) {
            std::cerr << "Failed to evaluate expression\n";
            return 1;
        }

        if (result != 42) {
            std::cerr << "Unexpected result: " << result << " (expected 42)\n";
            return 1;
        }

        std::cout << "Smoke test passed!\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}