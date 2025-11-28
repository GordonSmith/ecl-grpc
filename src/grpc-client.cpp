#include <google/protobuf/stubs/common.h>
#include <iostream>
#include <string>

#include "generated/addressbook.pb.h"

int test()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    tutorial::AddressBook book;

    tutorial::Person *alice = book.add_people();
    alice->set_name("Alice");
    alice->set_id(1);
    alice->set_email("alice@example.com");
    auto *alice_mobile = alice->add_phones();
    alice_mobile->set_number("+1-555-0100");
    alice_mobile->set_type(tutorial::Person::PHONE_TYPE_MOBILE);

    tutorial::Person *bob = book.add_people();
    bob->set_name("Bob");
    bob->set_id(2);
    auto *bob_home = bob->add_phones();
    bob_home->set_number("+1-555-0123");
    bob_home->set_type(tutorial::Person::PHONE_TYPE_HOME);

    std::string payload;
    if (!book.SerializeToString(&payload))
    {
        std::cerr << "Failed to serialize address book" << std::endl;
        return 1;
    }

    tutorial::AddressBook parsed;
    if (!parsed.ParseFromString(payload))
    {
        std::cerr << "Failed to parse serialized payload" << std::endl;
        return 1;
    }

    for (const auto &person : parsed.people())
    {
        std::cout << person.name() << " (id: " << person.id() << ")";
        if (!person.email().empty())
        {
            std::cout << ", email: " << person.email();
        }
        std::cout << '\n';
        for (const auto &phone : person.phones())
        {
            std::cout << "  - " << phone.number() << " [" << phone.type() << "]\n";
        }
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
