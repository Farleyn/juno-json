#include <iostream>
#include "juno.h"

int main() {
    juno::Value person;
    person["name"] = "Alice";
    person["age"] = 30;
    person["hobbies"] = juno::Array{"reading", "swimming"};

    std::cout << juno::stringify(person) << "\n";

    juno::Value parsed = juno::parse(juno::stringify(person));
    std::cout << "Name: " << parsed["name"].as_string() << "\n";
    std::cout << "Age: " << parsed["age"].as_int() << "\n";

    return 0;
}
