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

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "helper.h"
#ifdef BAZEL_BUILD
#include "examples/protos/route_guide.pb.h"
#include "examples/protos/route_guide.grpc.pb.h"
#else
#include "generated/route_guide.pb.h"
#include "generated/route_guide.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using routeguide::Feature;
using routeguide::Point;
using routeguide::Rectangle;
using routeguide::RouteGuide;
using routeguide::RouteNote;
using routeguide::RouteSummary;

Point MakePoint(long latitude, long longitude)
{
    Point p;
    p.set_latitude(latitude);
    p.set_longitude(longitude);
    return p;
}

Feature MakeFeature(const std::string &name, long latitude, long longitude)
{
    Feature f;
    f.set_name(name);
    f.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
    return f;
}

RouteNote MakeRouteNote(const std::string &message, long latitude,
                        long longitude)
{
    RouteNote n;
    n.set_message(message);
    n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
    return n;
}

class RouteGuideClient
{
public:
    RouteGuideClient(std::shared_ptr<Channel> channel, const std::string &db)
        : stub_(RouteGuide::NewStub(channel))
    {
        routeguide::ParseDb(db, &feature_list_);
    }

    void GetFeature()
    {
        Point point;
        Feature feature;
        point = MakePoint(409146138, -746188906);
        GetOneFeature(point, &feature);
        point = MakePoint(0, 0);
        GetOneFeature(point, &feature);
    }

    void ListFeatures()
    {
        routeguide::Rectangle rect;
        Feature feature;
        ClientContext context;

        rect.mutable_lo()->set_latitude(400000000);
        rect.mutable_lo()->set_longitude(-750000000);
        rect.mutable_hi()->set_latitude(420000000);
        rect.mutable_hi()->set_longitude(-730000000);
        std::cout << "Looking for features between 40, -75 and 42, -73"
                  << std::endl;

        std::unique_ptr<ClientReader<Feature>> reader(
            stub_->ListFeatures(&context, rect));
        while (reader->Read(&feature))
        {
            std::cout << "Found feature called " << feature.name() << " at "
                      << feature.location().latitude() / kCoordFactor_ << ", "
                      << feature.location().longitude() / kCoordFactor_ << std::endl;
        }
        Status status = reader->Finish();
        if (status.ok())
        {
            std::cout << "ListFeatures rpc succeeded." << std::endl;
        }
        else
        {
            std::cout << "ListFeatures rpc failed." << std::endl;
        }
    }

    void RecordRoute()
    {
        Point point;
        RouteSummary stats;
        ClientContext context;
        const int kPoints = 10;
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

        std::default_random_engine generator(seed);
        std::uniform_int_distribution<int> feature_distribution(
            0, feature_list_.size() - 1);
        std::uniform_int_distribution<int> delay_distribution(500, 1500);

        std::unique_ptr<ClientWriter<Point>> writer(
            stub_->RecordRoute(&context, &stats));
        for (int i = 0; i < kPoints; i++)
        {
            const Feature &f = feature_list_[feature_distribution(generator)];
            std::cout << "Visiting point " << f.location().latitude() / kCoordFactor_
                      << ", " << f.location().longitude() / kCoordFactor_
                      << std::endl;
            if (!writer->Write(f.location()))
            {
                // Broken stream.
                break;
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(delay_distribution(generator)));
        }
        writer->WritesDone();
        Status status = writer->Finish();
        if (status.ok())
        {
            std::cout << "Finished trip with " << stats.point_count() << " points\n"
                      << "Passed " << stats.feature_count() << " features\n"
                      << "Travelled " << stats.distance() << " meters\n"
                      << "It took " << stats.elapsed_time() << " seconds"
                      << std::endl;
        }
        else
        {
            std::cout << "RecordRoute rpc failed." << std::endl;
        }
    }

    void RouteChat()
    {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<RouteNote, RouteNote>> stream(
            stub_->RouteChat(&context));

        std::thread writer([stream]()
                           {
      std::vector<RouteNote> notes{MakeRouteNote("First message", 0, 0),
                                   MakeRouteNote("Second message", 0, 1),
                                   MakeRouteNote("Third message", 1, 0),
                                   MakeRouteNote("Fourth message", 0, 0)};
      for (const RouteNote& note : notes) {
        std::cout << "Sending message " << note.message() << " at "
                  << note.location().latitude() << ", "
                  << note.location().longitude() << std::endl;
        stream->Write(note);
      }
      stream->WritesDone(); });

        RouteNote server_note;
        while (stream->Read(&server_note))
        {
            std::cout << "Got message " << server_note.message() << " at "
                      << server_note.location().latitude() << ", "
                      << server_note.location().longitude() << std::endl;
        }
        writer.join();
        Status status = stream->Finish();
        if (!status.ok())
        {
            std::cout << "RouteChat rpc failed." << std::endl;
        }
    }

    std::size_t FeatureCount() const
    {
        return feature_list_.size();
    }

private:
    bool GetOneFeature(const Point &point, Feature *feature)
    {
        ClientContext context;
        Status status = stub_->GetFeature(&context, point, feature);
        if (!status.ok())
        {
            std::cout << "GetFeature rpc failed." << std::endl;
            return false;
        }
        if (!feature->has_location())
        {
            std::cout << "Server returns incomplete feature." << std::endl;
            return false;
        }
        if (feature->name().empty())
        {
            std::cout << "Found no feature at "
                      << feature->location().latitude() / kCoordFactor_ << ", "
                      << feature->location().longitude() / kCoordFactor_ << std::endl;
        }
        else
        {
            std::cout << "Found feature called " << feature->name() << " at "
                      << feature->location().latitude() / kCoordFactor_ << ", "
                      << feature->location().longitude() / kCoordFactor_ << std::endl;
        }
        return true;
    }

    const float kCoordFactor_ = 10000000.0;
    std::unique_ptr<RouteGuide::Stub> stub_;
    std::vector<Feature> feature_list_;
};

int test2()
{
    std::string db = routeguide::GetDbFileContent();
    RouteGuideClient guide(
        grpc::CreateChannel("localhost:50051",
                            grpc::InsecureChannelCredentials()),
        db);

    std::cout << "-------------- GetFeature --------------" << std::endl;
    guide.GetFeature();
    std::cout << "-------------- ListFeatures --------------" << std::endl;
    guide.ListFeatures();
    std::cout << "-------------- RecordRoute --------------" << std::endl;
    guide.RecordRoute();
    std::cout << "-------------- RouteChat --------------" << std::endl;
    guide.RouteChat();

    const auto loaded_features = guide.FeatureCount();
    std::cout << "Loaded " << loaded_features << " features from embedded DB" << std::endl;
    return static_cast<int>(loaded_features);
}
